import Foundation
import SwiftData

// MARK: - ModelConfig

/// Represents a downloaded on-device model (Whisper or Gemma GGUF).
/// Stored in SwiftData; model files live on disk, never in the DB.
@Model
public final class ModelConfig: @unchecked Sendable {
    public var id: UUID
    public var name: String              // e.g. "Whisper large-v3 q5_0"
    public var modelType: ModelType
    public var quantization: String      // e.g. "q5_0", "Q4_K_M"
    public var sizeBytes: Int64
    public var remoteURL: String         // Hugging Face direct download URL
    public var localFilename: String     // filename under …/models/
    public var sha256: String            // hex digest for verification
    public var isDefault: Bool
    public var downloadedAt: Date?

    public init(
        id: UUID = UUID(),
        name: String,
        modelType: ModelType,
        quantization: String,
        sizeBytes: Int64,
        remoteURL: String,
        localFilename: String,
        sha256: String,
        isDefault: Bool = false
    ) {
        self.id = id
        self.name = name
        self.modelType = modelType
        self.quantization = quantization
        self.sizeBytes = sizeBytes
        self.remoteURL = remoteURL
        self.localFilename = localFilename
        self.sha256 = sha256
        self.isDefault = isDefault
        self.downloadedAt = nil
    }

    /// Absolute path to the model file on disk.
    public var localURL: URL {
        ModelConfig.modelsDirectory.appendingPathComponent(localFilename)
    }

    /// True when the model file exists and matches expected size.
    public var isDownloaded: Bool {
        let url = localURL
        guard let attrs = try? FileManager.default.attributesOfItem(atPath: url.path) else { return false }
        let size = (attrs[.size] as? Int64) ?? 0
        return size > 0
    }

    // MARK: - Static helpers

    /// Base directory where model files are stored.
    public static var modelsDirectory: URL {
        #if os(macOS)
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        return appSupport.appendingPathComponent("llc.meridian.vox/models", isDirectory: true)
        #else
        // iOS: App Group container (Phase 2). For MVP use Documents.
        let documents = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        return documents.appendingPathComponent("vox/models", isDirectory: true)
        #endif
    }

    // MARK: - Bundled model catalog

    /// The default model set for Phase 1, keyed by tier.
    public static func defaultCatalog() -> [ModelConfig] {
        [
            // Whisper — Power tier (≥32GB Mac)
            ModelConfig(
                name: "Whisper large-v3 (q5_0)",
                modelType: .whisper,
                quantization: "q5_0",
                sizeBytes: 1_080_000_000,
                remoteURL: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3-q5_0.bin",
                localFilename: "ggml-large-v3-q5_0.bin",
                sha256: "",   // populated after first verified download
                isDefault: true
            ),
            // Whisper — Default tier (16GB Mac)
            ModelConfig(
                name: "Whisper medium.en (q5_0)",
                modelType: .whisper,
                quantization: "q5_0",
                sizeBytes: 539_000_000,
                remoteURL: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en-q5_0.bin",
                localFilename: "ggml-medium.en-q5_0.bin",
                sha256: "",
                isDefault: false
            ),
            // Whisper — Lite tier (8GB iOS/Mac)
            ModelConfig(
                name: "Whisper small.en (q5_1)",
                modelType: .whisper,
                quantization: "q5_1",
                sizeBytes: 190_000_000,
                remoteURL: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en-q5_1.bin",
                localFilename: "ggml-small.en-q5_1.bin",
                sha256: "",
                isDefault: false
            ),
            // Gemma 4 E4B — Power/Default tier Mac
            ModelConfig(
                name: "Gemma 4 E4B-it (Q4_K_M)",
                modelType: .gemma,
                quantization: "Q4_K_M",
                sizeBytes: 5_200_000_000,
                remoteURL: "https://huggingface.co/bartowski/google_gemma-4-E4B-it-GGUF/resolve/main/google_gemma-4-E4B-it-Q4_K_M.gguf",
                localFilename: "google_gemma-4-E4B-it-Q4_K_M.gguf",
                sha256: "",
                isDefault: true
            ),
            // Gemma 4 E2B — iOS / lower-RAM Mac
            ModelConfig(
                name: "Gemma 4 E2B-it (Q4_K_M)",
                modelType: .gemma,
                quantization: "Q4_K_M",
                sizeBytes: 2_200_000_000,
                remoteURL: "https://huggingface.co/bartowski/google_gemma-4-E2B-it-GGUF/resolve/main/google_gemma-4-E2B-it-Q4_K_M.gguf",
                localFilename: "google_gemma-4-E2B-it-Q4_K_M.gguf",
                sha256: "",
                isDefault: false
            )
        ]
    }
}

// MARK: - ModelType

public enum ModelType: String, Codable, Sendable {
    case whisper
    case gemma
}
