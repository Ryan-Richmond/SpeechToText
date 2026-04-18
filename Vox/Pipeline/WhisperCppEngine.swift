import Foundation
import OSLog
import whisper

/// STTEngine implementation using whisper.cpp via its C API.
///
/// Runs on a dedicated actor to ensure model access is serialized.
/// Metal acceleration is enabled by default on Apple Silicon.
actor WhisperCppEngine: STTEngine {

    private var context: OpaquePointer?

    func load(modelPath: URL) async throws {
        unloadSync()

        Log.stt.info("Loading Whisper model: \(modelPath.lastPathComponent)")

        var params = whisper_context_default_params()
        params.use_gpu = true

        let ctx = modelPath.path.withCString { cPath in
            whisper_init_from_file_with_params(cPath, params)
        }

        guard let ctx else {
            throw WhisperError.failedToLoad(modelPath.lastPathComponent)
        }

        context = ctx
        Log.stt.info("Whisper model loaded successfully")
    }

    func transcribe(
        samples: [Float],
        sampleRate: Double,
        hints: [String]
    ) async throws -> Transcript {
        guard let ctx = context else {
            throw WhisperError.modelNotLoaded
        }

        guard !samples.isEmpty else {
            return Transcript(text: "", segments: [], language: "en", durationMs: 0)
        }

        let startTime = CFAbsoluteTimeGetCurrent()

        var params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY)
        params.n_threads = Int32(min(ProcessInfo.processInfo.activeProcessorCount, 4))
        params.language = "en".withCString { strdup($0) }
        params.translate = false
        params.no_timestamps = false
        params.print_progress = false
        params.print_special = false
        params.print_realtime = false
        params.print_timestamps = false
        params.single_segment = false
        params.suppress_blank = true
        params.suppress_nst = true

        // Inject hints (personal dictionary) via initial_prompt.
        if !hints.isEmpty {
            let hintString = hints.joined(separator: ", ")
            params.initial_prompt = hintString.withCString { strdup($0) }
        }

        let result = samples.withUnsafeBufferPointer { buffer in
            whisper_full(ctx, params, buffer.baseAddress, Int32(buffer.count))
        }

        // Clean up strdup'd strings.
        if let lang = params.language { free(UnsafeMutablePointer(mutating: lang)) }
        if let prompt = params.initial_prompt { free(UnsafeMutablePointer(mutating: prompt)) }

        guard result == 0 else {
            throw WhisperError.transcriptionFailed(code: Int(result))
        }

        let segmentCount = whisper_full_n_segments(ctx)
        var segments: [Segment] = []
        var fullText = ""

        for i in 0..<segmentCount {
            guard let cText = whisper_full_get_segment_text(ctx, i) else { continue }
            let text = String(cString: cText)
            let t0 = Int(whisper_full_get_segment_t0(ctx, i)) * 10
            let t1 = Int(whisper_full_get_segment_t1(ctx, i)) * 10
            segments.append(Segment(text: text.trimmingCharacters(in: .whitespaces), startMs: t0, endMs: t1))
            fullText += text
        }

        let elapsedMs = Int((CFAbsoluteTimeGetCurrent() - startTime) * 1000)
        let transcript = fullText.trimmingCharacters(in: .whitespacesAndNewlines)
        let durationMs = Int(Double(samples.count) / sampleRate * 1000)

        Log.stt.info("Transcribed \(durationMs)ms audio in \(elapsedMs)ms: \(transcript.prefix(80), privacy: .private)...")

        return Transcript(text: transcript, segments: segments, language: "en", durationMs: durationMs)
    }

    func unload() async {
        unloadSync()
    }

    private func unloadSync() {
        if let ctx = context {
            whisper_free(ctx)
            context = nil
            Log.stt.info("Whisper model unloaded")
        }
    }

    deinit {
        if let ctx = context {
            whisper_free(ctx)
        }
    }
}

enum WhisperError: Error, LocalizedError {
    case failedToLoad(String)
    case modelNotLoaded
    case transcriptionFailed(code: Int)

    var errorDescription: String? {
        switch self {
        case .failedToLoad(let name): "Failed to load Whisper model: \(name)"
        case .modelNotLoaded: "No Whisper model is loaded. Call load() first."
        case .transcriptionFailed(let code): "Whisper transcription failed with code \(code)"
        }
    }
}
