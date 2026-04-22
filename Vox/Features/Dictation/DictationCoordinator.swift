import Foundation
import os

// MARK: - DictationState

public enum DictationState: Equatable, Sendable {
    case idle
    case recording
    case processing
    case done(text: String)
    case error(String)
}

// MARK: - DictationCoordinator

/// Drives the full dictation flow from hotkey/button → audio → pipeline → paste.
/// Observed by the overlay and menu bar icon for UI state.
@MainActor
@Observable
public final class DictationCoordinator {

    // MARK: - Singleton

    public static let shared = DictationCoordinator()
    private init() {}

    // MARK: - State

    public private(set) var state: DictationState = .idle
    public private(set) var rmsLevel: Float = 0

    private let logger = Logger.vox(.pipeline)
    private let audioCapture = AudioCapture()
    private var rmsPollingTask: Task<Void, Never>?

    // MARK: - Public API

    /// Begins recording. Called by hotkey on key-down.
    public func beginRecording() {
        guard case .idle = state else { return }
        state = .recording

        Task {
            do {
                try await audioCapture.startRecording()
                startRMSPolling()
                logger.info("DictationCoordinator: recording started")
            } catch {
                state = .error(error.localizedDescription)
                logger.error("DictationCoordinator: startRecording failed: \(error)")
            }
        }
    }

    /// Stops recording and runs the full pipeline. Called on hotkey key-up.
    public func endRecordingAndProcess() {
        guard case .recording = state else { return }
        stopRMSPolling()
        state = .processing

        Task {
            do {
                let audio = await audioCapture.stopRecording()
                guard audio.hasAudio else {
                    state = .idle
                    return
                }

                let context = AppContextService.currentContext()
                let result = try await PipelineActor.shared.dictate(audio: audio, context: context)

                // Paste into active text field
                try await PasteService.paste(text: result.cleanedText)

                state = .done(text: result.cleanedText)
                logger.info("DictationCoordinator: done — \(result.latencyMs)ms")

                // Save to history
                await saveTranscription(result: result)

                // Auto-reset to idle after 2s
                try await Task.sleep(nanoseconds: 2_000_000_000)
                if case .done = state { state = .idle }

            } catch {
                state = .error(error.localizedDescription)
                logger.error("DictationCoordinator: pipeline error: \(error)")
                try? await Task.sleep(nanoseconds: 3_000_000_000)
                if case .error = state { state = .idle }
            }
        }
    }

    /// Cancels any in-progress recording.
    public func cancel() {
        stopRMSPolling()
        Task {
            _ = await audioCapture.stopRecording()
            state = .idle
        }
    }

    // MARK: - Private helpers

    private func startRMSPolling() {
        rmsPollingTask = Task { [weak self] in
            while !Task.isCancelled {
                guard let self else { break }
                self.rmsLevel = await self.audioCapture.rmsLevel
                try? await Task.sleep(nanoseconds: 50_000_000) // 50ms
            }
        }
    }

    private func stopRMSPolling() {
        rmsPollingTask?.cancel()
        rmsPollingTask = nil
        rmsLevel = 0
    }

    private func saveTranscription(result: PipelineResult) async {
        // Persist to SwiftData — will be wired in Sprint 4
        // (Transcription entity not yet in model container)
        logger.debug("TODO: save transcription to SwiftData")
    }
}
