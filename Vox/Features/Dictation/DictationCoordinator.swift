import Foundation
import OSLog
import SwiftUI

@MainActor
final class DictationCoordinator: ObservableObject {

    enum State: Equatable {
        case idle
        case recording
        case processing
        case result(String)
        case error(String)
    }

    @Published private(set) var state: State = .idle
    @Published private(set) var lastTranscript: String = ""
    @Published private(set) var lastRawTranscript: String = ""
    @Published private(set) var latencyMs: Int = 0

    private let audioCapture = AudioCapture()
    private let sttEngine: any STTEngine
    private let downloadService = ModelDownloadService()
    private let permissions = PermissionsManager()

    private var modelLoaded = false

    init(sttEngine: any STTEngine = WhisperCppEngine()) {
        self.sttEngine = sttEngine
    }

    // MARK: - Lifecycle

    func setup() async {
        await permissions.checkAll()

        guard permissions.microphoneGranted || await permissions.requestMicrophone() else {
            state = .error("Microphone permission required.")
            return
        }

        do {
            try await ensureModelDownloaded()
        } catch {
            state = .error("Model download failed: \(error.localizedDescription)")
        }
    }

    // MARK: - Dictation

    func startDictation() async {
        guard state == .idle || state.isFinished else { return }

        do {
            try await audioCapture.startRecording()
            state = .recording
            Log.pipeline.info("Dictation recording started")
        } catch {
            state = .error("Mic error: \(error.localizedDescription)")
        }
    }

    func stopDictation() async {
        guard state == .recording else { return }

        state = .processing
        let startTime = CFAbsoluteTimeGetCurrent()
        let audio = await audioCapture.stopRecording()

        guard !audio.samples.isEmpty else {
            state = .idle
            return
        }

        do {
            if !modelLoaded {
                try await ensureModelDownloaded()
            }

            let transcript = try await sttEngine.transcribe(
                samples: audio.samples,
                sampleRate: audio.sampleRate,
                hints: []
            )

            lastRawTranscript = transcript.text
            // Sprint 2 adds Gemma cleanup here. For now, raw = final.
            lastTranscript = transcript.text
            latencyMs = Int((CFAbsoluteTimeGetCurrent() - startTime) * 1000)

            Log.pipeline.info("Dictation complete in \(self.latencyMs)ms")
            state = .result(transcript.text)
        } catch {
            state = .error("Transcription failed: \(error.localizedDescription)")
            Log.pipeline.error("Dictation failed: \(error.localizedDescription)")
        }
    }

    // MARK: - Private

    private func ensureModelDownloaded() async throws {
        let tier = DeviceCapabilityService.detectTier()
        let registry = try ModelRegistry.load()
        let (sttModel, _) = try registry.defaultModels(forTier: tier.rawValue)

        let modelDir = try await downloadService.modelDirectory()
        let isDownloaded = try await downloadService.isModelDownloaded(entry: sttModel)

        if !isDownloaded {
            Log.download.info("STT model not found, downloading \(sttModel.filename)")
            _ = try await downloadService.download(entry: sttModel, to: modelDir)
        }

        let modelPath = try await downloadService.modelPath(for: sttModel)
        try await sttEngine.load(modelPath: modelPath)
        modelLoaded = true
    }
}

private extension DictationCoordinator.State {
    var isFinished: Bool {
        switch self {
        case .result, .error: true
        default: false
        }
    }
}
