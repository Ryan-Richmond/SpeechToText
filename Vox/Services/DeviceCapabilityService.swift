import Foundation
import os

// MARK: - DeviceCapabilityService

/// Inspects physical RAM to pick the appropriate model tier.
/// Called once at first launch; user can override in Settings → Models.
public final class DeviceCapabilityService: Sendable {

    public static let shared = DeviceCapabilityService()
    private init() {}

    // MARK: - Tier

    public enum ModelTier: String, Sendable {
        /// ≥32 GB Mac: Whisper large-v3 + Gemma 4 E4B Q8_0
        case power
        /// 16 GB Mac / 8 GB iPhone Pro: Whisper medium.en + Gemma 4 E4B Q4_K_M
        case `default`
        /// 8 GB iPhone, older Mac: Whisper small.en + Gemma 4 E2B Q4_K_M
        case lite
        /// <8 GB: unsupported
        case unsupported
    }

    // MARK: - Public API

    public var tier: ModelTier {
        let gb = physicalMemoryGB
        switch gb {
        case 32...:    return .power
        case 16..<32:  return .default
        case 8..<16:   return .lite
        default:       return .unsupported
        }
    }

    public var physicalMemoryGB: Double {
        Double(ProcessInfo.processInfo.physicalMemory) / 1_073_741_824
    }

    /// Returns the recommended Whisper ModelConfig for the current tier.
    public func recommendedWhisperModel(from catalog: [ModelConfig]) -> ModelConfig? {
        let targetName: String
        switch tier {
        case .power:       targetName = "Whisper large-v3 (q5_0)"
        case .default:     targetName = "Whisper medium.en (q5_0)"
        case .lite:        targetName = "Whisper small.en (q5_1)"
        case .unsupported: return nil
        }
        return catalog.first { $0.name == targetName && $0.modelType == .whisper }
    }

    /// Returns the recommended Gemma ModelConfig for the current tier.
    public func recommendedGemmaModel(from catalog: [ModelConfig]) -> ModelConfig? {
        switch tier {
        case .power, .default:
            return catalog.first { $0.name == "Gemma 4 E4B-it (Q4_K_M)" }
        case .lite:
            return catalog.first { $0.name == "Gemma 4 E2B-it (Q4_K_M)" }
        case .unsupported:
            return nil
        }
    }
}
