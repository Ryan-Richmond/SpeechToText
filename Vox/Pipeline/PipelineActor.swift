import Foundation
import os

// MARK: - PipelineActor

/// The core inference pipeline.
/// Owns the STT and LLM engine lifecycle; dictation and command tasks
/// are serialized through this actor to prevent concurrent model use.
///
/// Architecture: serial, non-MainActor.
/// Only coordinators/views await it — never tap into it directly from UI.
public actor PipelineActor {

    // MARK: - Properties

    private let logger = Logger.vox(.pipeline)
    private let stt: any STTEngine
    private let llm: any LLMEngine
    private var isLoaded = false

    // MARK: - Singleton

    public static let shared: PipelineActor = {
        PipelineActor(
            stt: WhisperCppEngine(),
            llm: LlamaCppEngine()
        )
    }()

    // MARK: - Init

    public init(stt: some STTEngine, llm: some LLMEngine) {
        self.stt = stt
        self.llm = llm
    }

    // MARK: - Lifecycle

    /// Load both models. Called during onboarding or on first use.
    /// No-ops if already loaded.
    public func loadModels(whisper: ModelConfig, gemma: ModelConfig) async throws {
        guard !isLoaded else { return }
        logger.info("Loading pipeline models…")
        async let sttLoad: Void = stt.load(model: whisper)
        async let llmLoad: Void = llm.load(model: gemma)
        try await sttLoad
        try await llmLoad
        isLoaded = true
        logger.info("Pipeline ready")
    }

    /// Unload both models from memory.
    public func unloadModels() async {
        await stt.unload()
        await llm.unload()
        isLoaded = false
    }

    // MARK: - Dictation

    /// Full dictation pipeline: audio → STT → LLM cleanup → cleaned text.
    /// - Parameters:
    ///   - audio: The captured audio buffer.
    ///   - context: Active app context for prompt tuning (nil = generic).
    ///   - hints: User dictionary terms to hint Whisper with.
    /// - Returns: Cleaned, paste-ready text.
    public func dictate(
        audio: AudioBuffer,
        context: AppContext? = nil,
        hints: [String] = []
    ) async throws -> PipelineResult {
        guard isLoaded else { throw PipelineError.modelsNotLoaded }
        guard audio.hasAudio else { throw PipelineError.emptyAudio }

        logger.info("Pipeline: starting dictation (\(String(format: "%.1f", audio.durationSeconds))s audio)")
        let start = Date()

        // Stage 1: STT
        let transcript = try await stt.transcribe(audio: audio, hints: hints)
        logger.debug("STT: \"\(transcript.text.prefix(80))\"")

        guard !transcript.text.isEmpty else {
            throw PipelineError.emptyTranscript
        }

        // Stage 2: LLM cleanup
        let prompt = PromptBuilder.cleanupPrompt(rawTranscript: transcript.text, context: context)
        let cleaned = try await llm.generate(
            prompt: prompt,
            stops: ["<turn|>", "<|turn>"],
            maxTokens: 1024
        )

        let totalMs = Int(Date().timeIntervalSince(start) * 1_000)
        logger.info("Pipeline: dictation complete in \(totalMs)ms")

        return PipelineResult(
            rawTranscript: transcript.text,
            cleanedText: cleaned.isEmpty ? transcript.text : cleaned,
            latencyMs: totalMs,
            appContext: context
        )
    }

    // MARK: - Command Mode

    /// Command pipeline: selection + audio → STT for instruction → LLM rewrite.
    /// - Parameters:
    ///   - selection: The text the user had selected.
    ///   - audio: The spoken command/instruction.
    ///   - context: Active app context.
    public func command(
        selection: String,
        audio: AudioBuffer,
        context: AppContext? = nil
    ) async throws -> PipelineResult {
        guard isLoaded else { throw PipelineError.modelsNotLoaded }
        guard audio.hasAudio else { throw PipelineError.emptyAudio }
        guard !selection.isEmpty else { throw PipelineError.emptySelection }

        logger.info("Pipeline: starting command mode")
        let start = Date()

        // Stage 1: Transcribe the spoken instruction
        let instruction = try await stt.transcribe(audio: audio, hints: [])
        logger.debug("Command instruction: \"\(instruction.text)\"")

        // Stage 2: LLM command execution
        let prompt = PromptBuilder.commandPrompt(
            selection: selection,
            instruction: instruction.text,
            context: context
        )
        let result = try await llm.generate(
            prompt: prompt,
            stops: ["<turn|>", "<|turn>"],
            maxTokens: 2048
        )

        let totalMs = Int(Date().timeIntervalSince(start) * 1_000)
        logger.info("Pipeline: command complete in \(totalMs)ms")

        return PipelineResult(
            rawTranscript: instruction.text,
            cleanedText: result.isEmpty ? selection : result,
            latencyMs: totalMs,
            appContext: context
        )
    }
}

// MARK: - PipelineResult

public struct PipelineResult: Sendable {
    public let rawTranscript: String
    public let cleanedText: String
    public let latencyMs: Int
    public let appContext: AppContext?
}

// MARK: - PipelineError

public enum PipelineError: Error, LocalizedError {
    case modelsNotLoaded
    case emptyAudio
    case emptyTranscript
    case emptySelection

    public var errorDescription: String? {
        switch self {
        case .modelsNotLoaded:  return "Models are not loaded. Please wait for the download to finish."
        case .emptyAudio:       return "No audio was captured. Please try again and speak clearly."
        case .emptyTranscript:  return "Could not transcribe your speech. Please try again."
        case .emptySelection:   return "Please select text before using Command Mode."
        }
    }
}
