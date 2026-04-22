import OSLog

internal enum LogCategory: String {
    case app
    case audio
    case pipeline
    case models
    case stt
    case llm
    case download
    case permissions
    case ui
}

internal extension Logger {
    static func vox(_ category: LogCategory) -> Logger {
        Logger(subsystem: "llc.meridian.vox", category: category.rawValue)
    }
}
