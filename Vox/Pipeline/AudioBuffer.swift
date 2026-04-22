import Foundation
import AVFoundation

// MARK: - AudioBuffer

/// A captured audio buffer ready for STT processing.
/// Contains 16 kHz mono PCM Float32 samples.
public struct AudioBuffer: Sendable {
    public let samples: [Float]
    public let sampleRate: Double
    public let durationSeconds: Double

    public init(samples: [Float], sampleRate: Double = 16_000) {
        self.samples = samples
        self.sampleRate = sampleRate
        self.durationSeconds = Double(samples.count) / sampleRate
    }

    /// Returns true if the buffer has meaningful non-silent audio.
    public var hasAudio: Bool { !samples.isEmpty && durationSeconds > 0.1 }
}
