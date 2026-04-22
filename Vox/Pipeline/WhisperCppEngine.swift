import Foundation
import whisper
import os

// MARK: - WhisperCppEngine

/// Whisper STT engine backed by whisper.cpp with Metal acceleration.
/// Loads `.bin` models from the Vox models directory.
public actor WhisperCppEngine: STTEngine {

    private let logger = Logger.vox(.stt)
    nonisolated(unsafe) private var context: OpaquePointer?
    private var loadedModelPath: String?

    public init() {}

    // MARK: - STTEngine

    public func load(model: ModelConfig) async throws {
        let path = model.localURL.path

        // Already loaded?
        if path == loadedModelPath, context != nil { return }

        // Unload any existing model
        await unload()

        guard FileManager.default.fileExists(atPath: path) else {
            throw WhisperError.modelNotFound(path: path)
        }

        logger.info("Loading Whisper model: \(model.name)")

        var params = whisper_context_default_params()
        params.use_gpu = true   // Metal on Apple Silicon

        guard let ctx = whisper_init_from_file_with_params(path, params) else {
            throw WhisperError.modelLoadFailed(path: path)
        }

        self.context = ctx
        self.loadedModelPath = path
        logger.info("Whisper model loaded: \(model.name)")
    }

    public func transcribe(audio: AudioBuffer, hints: [String]) async throws -> Transcript {
        guard let ctx = context else {
            throw WhisperError.notLoaded
        }
        guard audio.hasAudio else {
            return Transcript(text: "", durationMs: 0, confidence: 0)
        }

        let startTime = Date()

        var params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY)
        params.print_progress = false
        params.print_realtime = false
        params.print_timestamps = false
        params.language = ("en" as NSString).utf8String
        params.n_threads = Int32(min(ProcessInfo.processInfo.processorCount, 8))

        // Inject vocabulary hints via initial_prompt
        let hintString = hints.isEmpty ? nil : hints.joined(separator: ", ")
        let hintCString = hintString.map { ($0 as NSString).utf8String }
        params.initial_prompt = hintCString ?? nil

        let samples = audio.samples
        let result = whisper_full(ctx, params, samples, Int32(samples.count))
        guard result == 0 else {
            throw WhisperError.transcriptionFailed(code: Int(result))
        }

        // Collect segments
        let nSegments = whisper_full_n_segments(ctx)
        var text = ""
        for i in 0..<nSegments {
            if let segText = whisper_full_get_segment_text(ctx, i) {
                text += String(cString: segText)
            }
        }

        let elapsed = Int(Date().timeIntervalSince(startTime) * 1_000)
        let cleanText = text.trimmingCharacters(in: .whitespacesAndNewlines)

        logger.debug("Whisper transcribed \(String(format: "%.1f", audio.durationSeconds))s audio in \(elapsed)ms: \"\(cleanText.prefix(60))\"")

        return Transcript(
            text: cleanText,
            durationMs: elapsed,
            confidence: 1.0
        )
    }

    public func unload() async {
        if let ctx = context {
            whisper_free(ctx)
            self.context = nil
            self.loadedModelPath = nil
            logger.info("Whisper model unloaded")
        }
    }

    deinit {
        if let ctx = context {
            whisper_free(ctx)
        }
    }
}

// MARK: - WhisperError

public enum WhisperError: Error, LocalizedError {
    case modelNotFound(path: String)
    case modelLoadFailed(path: String)
    case notLoaded
    case transcriptionFailed(code: Int)

    public var errorDescription: String? {
        switch self {
        case .modelNotFound(let path):
            return "Whisper model not found at \(path). Please download it first."
        case .modelLoadFailed(let path):
            return "Failed to load Whisper model from \(path). The file may be corrupted."
        case .notLoaded:
            return "Whisper model is not loaded. Call load() first."
        case .transcriptionFailed(let code):
            return "Whisper transcription failed with code \(code)."
        }
    }
}
