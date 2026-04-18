import Foundation

struct Transcript: Sendable {
    let text: String
    let segments: [Segment]
    let language: String
    let durationMs: Int
}

struct Segment: Sendable {
    let text: String
    let startMs: Int
    let endMs: Int
}

protocol STTEngine: Sendable {
    func load(modelPath: URL) async throws
    func transcribe(samples: [Float], sampleRate: Double, hints: [String]) async throws -> Transcript
    func unload() async
}
