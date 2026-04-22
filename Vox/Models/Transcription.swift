import Foundation
import SwiftData

// MARK: - Transcription

/// Records a completed dictation or command session.
/// Stored locally in SwiftData. Never leaves the device.
@Model
public final class Transcription {
    public var id: UUID
    public var rawText: String
    public var cleanedText: String
    public var appBundleID: String?
    public var appName: String?
    public var latencyMs: Int
    public var modelName: String
    public var createdAt: Date
    public var wasCommand: Bool

    public init(
        id: UUID = UUID(),
        rawText: String,
        cleanedText: String,
        appBundleID: String? = nil,
        appName: String? = nil,
        latencyMs: Int,
        modelName: String,
        wasCommand: Bool = false
    ) {
        self.id = id
        self.rawText = rawText
        self.cleanedText = cleanedText
        self.appBundleID = appBundleID
        self.appName = appName
        self.latencyMs = latencyMs
        self.modelName = modelName
        self.createdAt = Date()
        self.wasCommand = wasCommand
    }
}
