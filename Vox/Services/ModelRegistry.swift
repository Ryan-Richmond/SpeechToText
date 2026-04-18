import Foundation

enum ModelKind: String, Codable, Sendable {
    case stt
    case llm
}

struct ModelRegistryEntry: Codable, Sendable {
    let kind: ModelKind
    let framework: String
    let format: String
    let url: String
    let filename: String
    let sizeBytes: Int64
    let sha256: String
    let ramEstimateMb: Int
    let contextWindow: Int?
    let audioEncoder: Bool?
    let license: String
    let defaultForTiers: [String]

    enum CodingKeys: String, CodingKey {
        case kind, framework, format, url, filename
        case sizeBytes = "size_bytes"
        case sha256
        case ramEstimateMb = "ram_estimate_mb"
        case contextWindow = "context_window"
        case audioEncoder = "audio_encoder"
        case license
        case defaultForTiers = "default_for_tiers"
    }
}

struct TierDefinition: Codable, Sendable {
    let minTotalRamGb: Int
    let stt: String
    let llm: String

    enum CodingKeys: String, CodingKey {
        case minTotalRamGb = "min_total_ram_gb"
        case stt, llm
    }
}

private struct RegistryFile: Codable {
    let models: [String: ModelRegistryEntry]
    let tiers: [String: TierDefinition]
}

struct ModelRegistry: Sendable {

    let models: [String: ModelRegistryEntry]
    let tiers: [String: TierDefinition]

    static func load() throws -> ModelRegistry {
        guard let url = Bundle.main.url(forResource: "registry", withExtension: "json") else {
            throw ModelRegistryError.registryNotFound
        }
        let data = try Data(contentsOf: url)
        let file = try JSONDecoder().decode(RegistryFile.self, from: data)
        return ModelRegistry(models: file.models, tiers: file.tiers)
    }

    func defaultModels(forTier tierID: String) throws -> (stt: ModelRegistryEntry, llm: ModelRegistryEntry) {
        guard let tier = tiers[tierID] else {
            throw ModelRegistryError.unknownTier(tierID)
        }
        guard let stt = models[tier.stt] else {
            throw ModelRegistryError.unknownModel(tier.stt)
        }
        guard let llm = models[tier.llm] else {
            throw ModelRegistryError.unknownModel(tier.llm)
        }
        return (stt, llm)
    }

    func entry(for modelID: String) throws -> ModelRegistryEntry {
        guard let entry = models[modelID] else {
            throw ModelRegistryError.unknownModel(modelID)
        }
        return entry
    }
}

enum ModelRegistryError: Error, LocalizedError {
    case registryNotFound
    case unknownTier(String)
    case unknownModel(String)

    var errorDescription: String? {
        switch self {
        case .registryNotFound:
            "Model registry (registry.json) not found in app bundle."
        case .unknownTier(let id):
            "Unknown device tier: \(id)"
        case .unknownModel(let id):
            "Unknown model identifier: \(id)"
        }
    }
}
