import OSLog

internal enum LogCategory: String {
    case app
    case audio
    case pipeline
    case models
}

internal extension Logger {
    static func vox(_ category: LogCategory) -> Logger {
        Logger(subsystem: "llc.meridian.vox", category: category.rawValue)
    }
}
