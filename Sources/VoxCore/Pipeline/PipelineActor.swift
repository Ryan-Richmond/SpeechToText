import Foundation

public actor PipelineActor {
    public init() {}

    public func dictate(transcript: String) -> String {
        let trimmed = transcript.trimmingCharacters(in: .whitespacesAndNewlines)
        guard trimmed.isEmpty == false else {
            return ""
        }

        VoxLogger.info("Cleanup requested for transcript length: \(trimmed.count)")
        return trimmed
    }
}
