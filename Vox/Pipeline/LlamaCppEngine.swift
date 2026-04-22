import Foundation
import llama
import os

// MARK: - LlamaCppEngine

/// Gemma LLM engine backed by llama.cpp with Metal acceleration.
/// Loads GGUF model files from the Vox models directory.
public actor LlamaCppEngine: LLMEngine {

    private let logger = Logger.vox(.llm)
    nonisolated(unsafe) private var model: OpaquePointer?
    nonisolated(unsafe) private var ctx: OpaquePointer?
    private var loadedModelPath: String?

    // Default context size — 4096 tokens is plenty for cleanup tasks
    private let contextSize: Int32 = 4096

    public init() {}

    // MARK: - LLMEngine

    public func load(model config: ModelConfig) async throws {
        let path = config.localURL.path

        if path == loadedModelPath, model != nil { return }

        await unload()

        guard FileManager.default.fileExists(atPath: path) else {
            throw LlamaError.modelNotFound(path: path)
        }

        logger.info("Loading LLM model: \(config.name)")

        var modelParams = llama_model_default_params()
        modelParams.n_gpu_layers = 99   // all layers on Metal

        guard let loadedModel = llama_model_load_from_file(path, modelParams) else {
            throw LlamaError.modelLoadFailed(path: path)
        }

        var ctxParams = llama_context_default_params()
        ctxParams.n_ctx = UInt32(contextSize)
        ctxParams.n_batch = 512

        guard let loadedCtx = llama_new_context_with_model(loadedModel, ctxParams) else {
            llama_model_free(loadedModel)
            throw LlamaError.contextCreationFailed
        }

        self.model = loadedModel
        self.ctx = loadedCtx
        self.loadedModelPath = path
        logger.info("LLM model loaded: \(config.name)")
    }

    public func generate(prompt: Prompt, stops: [String], maxTokens: Int) async throws -> String {
        guard let model, let ctx else {
            throw LlamaError.notLoaded
        }

        // Build a Gemma 4 chat-formatted prompt
        // Format: <bos><|turn>system {system}<turn|> <|turn>user {user}<turn|> <|turn>model
        let formatted = "<bos><|turn>system \(prompt.system)<turn|> <|turn>user \(prompt.user)<turn|> <|turn>model"

        let startTime = Date()

        // Tokenise
        var tokens = [llama_token](repeating: 0, count: Int(contextSize))
        let nTokens = llama_tokenize(model, formatted, Int32(formatted.utf8.count), &tokens, Int32(contextSize), true, true)
        guard nTokens > 0 else {
            throw LlamaError.tokenizationFailed
        }

        // Evaluate prompt tokens — size batch to actual token count to avoid buffer overrun
        var batch = llama_batch_init(nTokens, 0, 1)
        defer { llama_batch_free(batch) }

        for (i, token) in tokens[0..<Int(nTokens)].enumerated() {
            // llama_batch_add logic
            let idx = Int(batch.n_tokens)
            batch.token[idx] = token
            batch.pos[idx] = Int32(i)
            batch.n_seq_id[idx] = 1
            batch.seq_id[idx]![0] = 0
            batch.logits[idx] = 0
            batch.n_tokens += 1
        }

        // Last token needs logits
        batch.logits[Int(batch.n_tokens) - 1] = 1

        guard llama_decode(ctx, batch) == 0 else {
            throw LlamaError.decodeFailed
        }

        // Greedy decode loop
        var output = ""
        var nDecoded = 0
        var nPos = nTokens

        while nDecoded < maxTokens {
            let logits = llama_get_logits_ith(ctx, batch.n_tokens - 1)!
            let nVocab = llama_vocab_n_tokens(model)

            // Greedy: pick argmax
            var maxLogit: Float = -Float.infinity
            var nextToken: llama_token = 0
            for i in 0..<Int(nVocab) {
                if logits[i] > maxLogit {
                    maxLogit = logits[i]
                    nextToken = llama_token(i)
                }
            }

            // EOS check
            if llama_vocab_is_eog(model, nextToken) { break }

            // Detokenize
            var tokenStr = [CChar](repeating: 0, count: 256)
            let len = llama_token_to_piece(model, nextToken, &tokenStr, 256, 0, false)
            if len > 0 {
                let piece = String(bytes: tokenStr.prefix(Int(len)).map { UInt8(bitPattern: $0) }, encoding: .utf8) ?? ""
                output += piece

                // Stop sequence check
                var hitStop = false
                for stop in stops {
                    if output.hasSuffix(stop) {
                        output = String(output.dropLast(stop.count))
                        hitStop = true
                        break
                    }
                }
                if hitStop { break }
            }

            // Next decode step
            batch.n_tokens = 0 // llama_batch_clear
            
            // llama_batch_add
            let idx = Int(batch.n_tokens)
            batch.token[idx] = nextToken
            batch.pos[idx] = nPos
            batch.n_seq_id[idx] = 1
            batch.seq_id[idx]![0] = 0
            batch.logits[idx] = 1
            batch.n_tokens += 1
            
            nPos += 1

            guard llama_decode(ctx, batch) == 0 else { break }
            nDecoded += 1
        }

        llama_memory_seq_rm(llama_get_memory(ctx), 0, 0, -1) // Clear cache for seq 0

        let elapsed = Int(Date().timeIntervalSince(startTime) * 1_000)
        logger.debug("LLM generated \(nDecoded) tokens in \(elapsed)ms")

        return output.trimmingCharacters(in: .whitespacesAndNewlines)
    }

    public func unload() async {
        if let ctx {
            llama_free(ctx)
            self.ctx = nil
        }
        if let model {
            llama_model_free(model)
            self.model = nil
        }
        loadedModelPath = nil
        logger.info("LLM model unloaded")
    }

    deinit {
        if let ctx { llama_free(ctx) }
        if let model { llama_model_free(model) }
    }
}

// MARK: - LlamaError

public enum LlamaError: Error, LocalizedError {
    case modelNotFound(path: String)
    case modelLoadFailed(path: String)
    case contextCreationFailed
    case notLoaded
    case tokenizationFailed
    case decodeFailed

    public var errorDescription: String? {
        switch self {
        case .modelNotFound(let path):   return "Gemma model not found at \(path). Please download it."
        case .modelLoadFailed(let path): return "Failed to load Gemma model from \(path). File may be corrupted."
        case .contextCreationFailed:     return "Could not create llama.cpp inference context."
        case .notLoaded:                 return "LLM model is not loaded. Call load() first."
        case .tokenizationFailed:        return "Failed to tokenize prompt."
        case .decodeFailed:              return "LLM decode step failed."
        }
    }
}
