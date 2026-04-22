import Foundation

// MARK: - STTEngine Protocol

/// Speech-to-text engine abstraction.
/// Phase 1 implementation: WhisperCppEngine.
/// Phase 2: CoreMLWhisperEngine.
public protocol STTEngine: Sendable {
    /// Load the model from disk. Must be called before transcribe().
    func load(model: ModelConfig) async throws

    /// Transcribe a PCM audio buffer into a Transcript.
    /// - Parameters:
    ///   - audio: The captured audio.
    ///   - hints: Optional vocabulary hints (user dictionary terms).
    func transcribe(audio: AudioBuffer, hints: [String]) async throws -> Transcript

    /// Unload the model from memory.
    func unload() async
}

// MARK: - Transcript

/// The output of a successful transcription.
public struct Transcript: Sendable {
    public let text: String
    public let language: String
    public let durationMs: Int
    public let confidence: Float     // 0.0 – 1.0, estimated from avg log-prob

    public init(text: String, language: String = "en", durationMs: Int, confidence: Float = 1.0) {
        self.text = text
        self.language = language
        self.durationMs = durationMs
        self.confidence = confidence
    }
}
