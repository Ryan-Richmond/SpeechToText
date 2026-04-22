import Foundation

// MARK: - LLMEngine Protocol

/// Large language model engine abstraction.
/// Phase 1 implementation: LlamaCppEngine.
/// Phase 2: CoreMLGemmaEngine, CloudLLMEngine (BYOK).
public protocol LLMEngine: Sendable {
    /// Load the GGUF model from disk.
    func load(model: ModelConfig) async throws

    /// Generate a completion for the given prompt.
    /// - Parameters:
    ///   - prompt: System + user prompt pair.
    ///   - stops: Stop sequences to halt generation.
    ///   - maxTokens: Maximum tokens to generate.
    func generate(prompt: Prompt, stops: [String], maxTokens: Int) async throws -> String

    /// Unload the model from memory.
    func unload() async
}

// MARK: - Prompt

/// A system + user prompt pair for the LLM.
public struct Prompt: Sendable {
    public let system: String
    public let user: String

    public init(system: String, user: String) {
        self.system = system
        self.user = user
    }
}
