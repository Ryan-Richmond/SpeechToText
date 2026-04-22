import Foundation
import AVFoundation
import os

// MARK: - AudioCapture

/// Captures microphone audio using AVAudioEngine and converts it to
/// 16 kHz mono Float32 PCM suitable for Whisper.
///
/// Usage:
///   let capture = AudioCapture()
///   try await capture.startRecording()
///   // … user speaks …
///   let buffer = await capture.stopRecording()
public actor AudioCapture {

    // MARK: - Properties

    private let logger = Logger.vox(.audio)

    private let engine = AVAudioEngine()
    private var isRecording = false
    private var collectedSamples: [Float] = []

    /// RMS level published for waveform animation (0.0 – 1.0).
    /// Callers can poll this while recording.
    public private(set) var rmsLevel: Float = 0

    // MARK: - Target audio format: 16 kHz mono Float32

    private static let targetSampleRate: Double = 16_000
    private static let targetFormat = AVAudioFormat(
        commonFormat: .pcmFormatFloat32,
        sampleRate: targetSampleRate,
        channels: 1,
        interleaved: false
    )!

    // MARK: - Constants

    /// RMS threshold below which audio is considered silence.
    private static let silenceThreshold: Float = 0.01

    final class TapData: @unchecked Sendable {
        let buffer: AVAudioPCMBuffer
        let converter: AVAudioConverter
        let inputFormat: AVAudioFormat
        init(buffer: AVAudioPCMBuffer, converter: AVAudioConverter, inputFormat: AVAudioFormat) {
            self.buffer = buffer
            self.converter = converter
            self.inputFormat = inputFormat
        }
    }

    // MARK: - Public API

    /// Requests microphone permission and starts capturing audio.
    /// Throws if permission is denied or the engine fails to start.
    public func startRecording() async throws {
        guard !isRecording else { return }

        try await requestMicrophonePermission()

        let inputNode = engine.inputNode
        let inputFormat = inputNode.outputFormat(forBus: 0)

        guard let converter = AVAudioConverter(from: inputFormat, to: Self.targetFormat) else {
            throw AudioCaptureError.formatConversionFailed
        }

        collectedSamples = []
        collectedSamples.reserveCapacity(160_000) // ~10s pre-alloc

        inputNode.installTap(onBus: 0, bufferSize: 4096, format: inputFormat) { [weak self] buffer, _ in
            guard let self else { return }
            let data = TapData(buffer: buffer, converter: converter, inputFormat: inputFormat)
            Task { await self.processTap(data: data) }
        }

        engine.prepare()
        try engine.start()
        isRecording = true
        logger.info("AudioCapture: recording started")
    }

    /// Stops recording and returns the collected AudioBuffer.
    /// Applies simple RMS-based VAD to trim leading/trailing silence.
    public func stopRecording() async -> AudioBuffer {
        guard isRecording else { return AudioBuffer(samples: []) }
        isRecording = false

        engine.inputNode.removeTap(onBus: 0)
        engine.stop()

        let trimmed = trimSilence(from: collectedSamples)
        logger.info("AudioCapture: stopped. Duration: \(String(format: "%.1f", Double(trimmed.count) / 16_000))s, samples: \(trimmed.count)")
        return AudioBuffer(samples: trimmed)
    }

    // MARK: - Private helpers

    private func processTap(data: TapData) async {
        guard isRecording else { return }
        let buffer = data.buffer
        let converter = data.converter
        let inputFormat = data.inputFormat

        // Convert to 16kHz mono
        let frameCapacity = AVAudioFrameCount(
            Double(buffer.frameLength) * Self.targetSampleRate / inputFormat.sampleRate
        )
        guard frameCapacity > 0,
              let outputBuffer = AVAudioPCMBuffer(pcmFormat: Self.targetFormat, frameCapacity: frameCapacity) else {
            return
        }

        final class UnsafeBufferWrapper: @unchecked Sendable {
            let buffer: AVAudioPCMBuffer
            init(_ buffer: AVAudioPCMBuffer) { self.buffer = buffer }
        }
        let wrapper = UnsafeBufferWrapper(buffer)

        var error: NSError?
        let inputBlock: AVAudioConverterInputBlock = { _, outStatus in
            outStatus.pointee = .haveData
            return wrapper.buffer
        }

        converter.convert(to: outputBuffer, error: &error, withInputFrom: inputBlock)

        guard error == nil,
              let channelData = outputBuffer.floatChannelData?[0] else { return }

        let count = Int(outputBuffer.frameLength)
        let newSamples = Array(UnsafeBufferPointer(start: channelData, count: count))

        // Update RMS level for waveform display
        let rms = sqrt(newSamples.reduce(0) { $0 + $1 * $1 } / Float(max(count, 1)))
        rmsLevel = rms

        collectedSamples.append(contentsOf: newSamples)
    }

    private func trimSilence(from samples: [Float]) -> [Float] {
        guard !samples.isEmpty else { return [] }

        let windowSize = 1_600 // 100ms windows at 16kHz

        // Find first non-silent window
        var start = 0
        for i in stride(from: 0, to: samples.count - windowSize, by: windowSize) {
            let chunk = samples[i..<(i + windowSize)]
            let rms = sqrt(chunk.reduce(0) { $0 + $1 * $1 } / Float(windowSize))
            if rms > Self.silenceThreshold {
                start = max(0, i - windowSize) // keep a tiny pre-roll
                break
            }
        }

        // Find last non-silent window
        var end = samples.count
        for i in stride(from: samples.count - windowSize, through: 0, by: -windowSize) {
            let chunk = samples[i..<(i + windowSize)]
            let rms = sqrt(chunk.reduce(0) { $0 + $1 * $1 } / Float(windowSize))
            if rms > Self.silenceThreshold {
                end = min(samples.count, i + windowSize * 2)
                break
            }
        }

        guard start < end else { return [] }
        return Array(samples[start..<end])
    }

    private func requestMicrophonePermission() async throws {
        #if os(macOS)
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        switch status {
        case .authorized: return
        case .notDetermined:
            let granted = await AVCaptureDevice.requestAccess(for: .audio)
            if !granted { throw AudioCaptureError.microphonePermissionDenied }
        default:
            throw AudioCaptureError.microphonePermissionDenied
        }
        #else
        let status = AVAudioApplication.shared.recordPermission
        switch status {
        case .granted: return
        case .undetermined:
            let granted = await AVAudioApplication.requestRecordPermission()
            if !granted { throw AudioCaptureError.microphonePermissionDenied }
        default:
            throw AudioCaptureError.microphonePermissionDenied
        }
        #endif
    }
}

// MARK: - AudioCaptureError

public enum AudioCaptureError: Error, LocalizedError {
    case microphonePermissionDenied
    case formatConversionFailed
    case engineStartFailed(Error)

    public var errorDescription: String? {
        switch self {
        case .microphonePermissionDenied:
            return "Vox needs microphone access to transcribe your speech. Please grant access in System Settings."
        case .formatConversionFailed:
            return "Could not configure audio format. Please restart the app."
        case .engineStartFailed(let e):
            return "Audio engine failed to start: \(e.localizedDescription)"
        }
    }
}
