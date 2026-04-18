import AVFoundation
import OSLog

actor AudioCapture {

    struct AudioBuffer: Sendable {
        let samples: [Float]
        let sampleRate: Double
        let durationSeconds: Double
    }

    private var engine: AVAudioEngine?
    private var accumulatedSamples: [Float] = []
    private let targetSampleRate: Double = 16_000

    private var isRecording = false

    // MARK: - Public

    func startRecording() async throws {
        guard !isRecording else { return }

        let session = AVAudioSession.sharedInstance()
        try session.setCategory(.record, mode: .measurement)
        try session.setActive(true)

        let engine = AVAudioEngine()
        let inputNode = engine.inputNode
        let inputFormat = inputNode.outputFormat(forBus: 0)

        accumulatedSamples.removeAll()

        let converter = makeConverter(from: inputFormat)

        inputNode.installTap(onBus: 0, bufferSize: 4096, format: inputFormat) { [weak self] buffer, _ in
            guard let self else { return }
            let converted = self.convertToMono16k(buffer: buffer, converter: converter)
            Task { await self.appendSamples(converted) }
        }

        try engine.start()
        self.engine = engine
        isRecording = true
        Log.audio.info("Recording started (input: \(Int(inputFormat.sampleRate)) Hz → 16 kHz mono)")
    }

    func stopRecording() async -> AudioBuffer {
        guard isRecording, let engine else {
            return AudioBuffer(samples: [], sampleRate: targetSampleRate, durationSeconds: 0)
        }

        engine.inputNode.removeTap(onBus: 0)
        engine.stop()
        self.engine = nil
        isRecording = false

        let samples = trimSilence(from: accumulatedSamples)
        let duration = Double(samples.count) / targetSampleRate
        Log.audio.info("Recording stopped: \(String(format: "%.1f", duration))s, \(samples.count) samples")

        return AudioBuffer(samples: samples, sampleRate: targetSampleRate, durationSeconds: duration)
    }

    // MARK: - Private

    private func appendSamples(_ newSamples: [Float]) {
        accumulatedSamples.append(contentsOf: newSamples)
    }

    private func makeConverter(from inputFormat: AVAudioFormat) -> AVAudioConverter? {
        guard let outputFormat = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: targetSampleRate,
            channels: 1,
            interleaved: false
        ) else { return nil }

        return AVAudioConverter(from: inputFormat, to: outputFormat)
    }

    private nonisolated func convertToMono16k(buffer: AVAudioPCMBuffer, converter: AVAudioConverter?) -> [Float] {
        guard let converter,
              let outputFormat = AVAudioFormat(
                  commonFormat: .pcmFormatFloat32,
                  sampleRate: targetSampleRate,
                  channels: 1,
                  interleaved: false
              ) else {
            return extractMono(from: buffer)
        }

        let ratio = targetSampleRate / buffer.format.sampleRate
        let outputFrameCount = AVAudioFrameCount(Double(buffer.frameLength) * ratio)
        guard let outputBuffer = AVAudioPCMBuffer(pcmFormat: outputFormat, frameCapacity: outputFrameCount) else {
            return extractMono(from: buffer)
        }

        var error: NSError?
        converter.convert(to: outputBuffer, error: &error) { _, outStatus in
            outStatus.pointee = .haveData
            return buffer
        }

        if let error {
            Log.audio.error("Audio conversion failed: \(error.localizedDescription)")
            return extractMono(from: buffer)
        }

        guard let channelData = outputBuffer.floatChannelData else { return [] }
        return Array(UnsafeBufferPointer(start: channelData[0], count: Int(outputBuffer.frameLength)))
    }

    private nonisolated func extractMono(from buffer: AVAudioPCMBuffer) -> [Float] {
        guard let channelData = buffer.floatChannelData else { return [] }
        return Array(UnsafeBufferPointer(start: channelData[0], count: Int(buffer.frameLength)))
    }

    /// Simple VAD: strip leading/trailing silence below an RMS threshold.
    private func trimSilence(from samples: [Float], windowSize: Int = 1600, threshold: Float = 0.01) -> [Float] {
        guard samples.count > windowSize else { return samples }

        func rms(_ slice: ArraySlice<Float>) -> Float {
            let sumSq = slice.reduce(Float(0)) { $0 + $1 * $1 }
            return (sumSq / Float(slice.count)).squareRoot()
        }

        var start = 0
        while start + windowSize <= samples.count {
            if rms(samples[start..<(start + windowSize)]) > threshold { break }
            start += windowSize
        }

        var end = samples.count
        while end - windowSize >= start {
            if rms(samples[(end - windowSize)..<end]) > threshold { break }
            end -= windowSize
        }

        guard start < end else { return [] }
        return Array(samples[start..<end])
    }
}
