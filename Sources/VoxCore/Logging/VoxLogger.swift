#if canImport(OSLog)
import OSLog
#endif

public enum VoxLogger {
    #if canImport(OSLog)
    private static let subsystem = "llc.meridian.vox"
    private static let category = "pipeline"

    private static let logger = Logger(subsystem: subsystem, category: category)
    #endif

    public static func info(_ message: String) {
        #if canImport(OSLog)
        logger.info("\(message, privacy: .public)")
        #else
        print("[INFO] \(message)")
        #endif
    }
}
