import Foundation
import SwiftData

// MARK: - DictionaryEntry

/// A user-defined word or phrase that Whisper should recognize correctly.
/// Injected into Whisper's initial_prompt hint.
@Model
public final class DictionaryEntry {
    public var id: UUID
    public var term: String           // The correct spelling/form
    public var pronunciationHint: String?   // Optional phonetic hint
    public var source: EntrySource
    public var createdAt: Date

    public init(
        id: UUID = UUID(),
        term: String,
        pronunciationHint: String? = nil,
        source: EntrySource = .manual
    ) {
        self.id = id
        self.term = term
        self.pronunciationHint = pronunciationHint
        self.source = source
        self.createdAt = Date()
    }
}

public enum EntrySource: String, Codable, Sendable {
    case manual      // Added by user in Settings
    case autoLearned // Suggested from transcription corrections (Phase 2)
}

// MARK: - StyleProfile

/// Placeholder for Phase 2 tone-matching. Stored but unused in Phase 1.
@Model
public final class StyleProfile {
    public var id: UUID
    public var avgSentenceLengthWords: Double
    public var updatedAt: Date

    public init() {
        self.id = UUID()
        self.avgSentenceLengthWords = 15
        self.updatedAt = Date()
    }
}
