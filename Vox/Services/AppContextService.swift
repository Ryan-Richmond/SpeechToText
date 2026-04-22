import Foundation
import os
#if os(macOS)
import AppKit
#endif

// MARK: - AppContextService

/// Detects the frontmost application on macOS to inject context into prompts.
/// On iOS, context is not available without accessibility APIs (Phase 2).
public struct AppContextService: Sendable {

    private static let logger = Logger.vox(.pipeline)

    // MARK: - Public API

    /// Returns the current AppContext based on the frontmost application.
    /// Returns nil on iOS or if detection fails.
    public static func currentContext() -> AppContext? {
        #if os(macOS)
        return macOSContext()
        #else
        return nil
        #endif
    }

    // MARK: - macOS

    #if os(macOS)
    private static func macOSContext() -> AppContext? {
        guard let app = NSWorkspace.shared.frontmostApplication else { return nil }
        let bundleID = app.bundleIdentifier ?? ""
        let appName  = app.localizedName ?? "Unknown"
        let family   = appFamily(for: bundleID)
        logger.debug("Active app: \(appName) (\(bundleID)) → family: \(family.rawValue)")
        return AppContext(bundleID: bundleID, appFamily: family, appName: appName)
    }

    private static func appFamily(for bundleID: String) -> AppFamily {
        switch bundleID {
        // Mail clients
        case "com.apple.mail",
             "com.microsoft.Outlook",
             "com.readdle.smartemail-mobile",
             "com.sparrowmailapp.sparrow",
             "ru.keepsolid.spark-desktop":
            return .mail

        // Messaging / collaboration
        case "com.apple.MobileSMS",
             "com.tinyspeck.slackmacgap",
             "com.microsoft.teams",
             "com.microsoft.teams2",
             "com.discord",
             "net.whatsapp.WhatsApp",
             "com.facebook.archon.developerID",
             "com.telegram.desktop":
            return .messages

        // Code editors / terminals
        case "com.apple.dt.Xcode",
             "com.microsoft.VSCode",
             "com.todesktop.230313mzl4w4u92",  // Cursor
             "com.github.atom",
             "com.jetbrains.intellij",
             "com.apple.Terminal",
             "com.googlecode.iterm2",
             "dev.warp.Warp-Stable":
            return .code

        // Notes / writing
        case "com.apple.Notes",
             "notion.id",
             "md.obsidian",
             "net.shinyfrog.bear",
             "com.pockity.app":
            return .notes

        // Browsers
        case "com.apple.Safari",
             "com.google.Chrome",
             "org.mozilla.firefox",
             "company.thebrowser.Browser",
             "com.microsoft.edgemac":
            return .browser

        // Word processors
        case "com.apple.iWork.Pages",
             "com.microsoft.Word",
             "com.google.Chrome.app.aohghmighlieiainnegkcijnfilokake": // Docs
            return .docs

        default:
            return .generic
        }
    }
    #endif
}
