import Foundation
import OSLog

/// Centralized `Logger` instances for Vox.
///
/// Subsystem is fixed to `llc.meridian.vox`. Categories match the buckets
/// described in `CLAUDE.md` §Coding Conventions.
///
/// **Privacy:** Never log audio buffers, raw transcripts, cleaned text, or
/// API keys at any level above `.debug`. The `vox-privacy-audit` skill
/// enforces this.
public enum Log {

    private static let subsystem = "llc.meridian.vox"

    /// Pipeline orchestration: state transitions, timing, lifecycle.
    public static let pipeline    = Logger(subsystem: subsystem, category: "pipeline")

    /// Audio capture, VAD, buffer management.
    public static let audio       = Logger(subsystem: subsystem, category: "audio")

    /// STT engine (whisper.cpp / Core ML).
    public static let stt         = Logger(subsystem: subsystem, category: "stt")

    /// LLM engine (llama.cpp / Core ML / cloud BYOK).
    public static let llm         = Logger(subsystem: subsystem, category: "llm")

    /// UI lifecycle, view state, user interactions.
    public static let ui          = Logger(subsystem: subsystem, category: "ui")

    /// Model registry + first-launch download.
    public static let download    = Logger(subsystem: subsystem, category: "download")

    /// Permissions: microphone, accessibility.
    public static let permissions = Logger(subsystem: subsystem, category: "permissions")

    /// Hotkey registration + global event monitoring (macOS).
    public static let hotkey      = Logger(subsystem: subsystem, category: "hotkey")
}
