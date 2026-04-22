import Foundation
import os
#if os(macOS)
import AppKit
import CoreGraphics
#else
import UIKit
#endif

// MARK: - PasteService

/// Writes cleaned text to the system pasteboard and simulates paste.
///
/// macOS: Uses NSPasteboard + CGEvent to synthesize Cmd+V in the frontmost app.
/// iOS MVP: Writes to UIPasteboard; user pastes manually (keyboard extension in v1.1).
public struct PasteService: Sendable {

    private static let logger = Logger.vox(.pipeline)

    // MARK: - Public API

    /// Pastes the given text at the current cursor position.
    /// On macOS, saves and restores the existing clipboard contents.
    public static func paste(text: String) async throws {
        guard !text.isEmpty else { return }

        #if os(macOS)
        try await pasteOnMac(text: text)
        #else
        pasteOnIOS(text: text)
        #endif
    }

    // MARK: - macOS implementation

    #if os(macOS)
    private static func pasteOnMac(text: String) async throws {
        let pasteboard = NSPasteboard.general

        // Save existing clipboard
        let savedContents: [(NSPasteboard.PasteboardType, Data)] = pasteboard.pasteboardItems?.flatMap { item in
            item.types.compactMap { type in
                item.data(forType: type).map { (type, $0) }
            }
        } ?? []

        // Write our text
        pasteboard.clearContents()
        pasteboard.setString(text, forType: .string)

        // Synthesize Cmd+V
        try synthesizeCmdV()

        // Brief delay then restore pasteboard
        try await Task.sleep(nanoseconds: 300_000_000) // 300ms
        if !savedContents.isEmpty {
            pasteboard.clearContents()
            let item = NSPasteboardItem()
            for (type, data) in savedContents {
                item.setData(data, forType: type)
            }
            pasteboard.writeObjects([item])
        }

        logger.info("Pasted \(text.count) chars via Cmd+V")
    }

    private static func synthesizeCmdV() throws {
        // Key code 9 = 'v'
        let src = CGEventSource(stateID: .hidSystemState)
        guard
            let keyDown = CGEvent(keyboardEventSource: src, virtualKey: 0x09, keyDown: true),
            let keyUp   = CGEvent(keyboardEventSource: src, virtualKey: 0x09, keyDown: false)
        else { throw PasteError.eventCreationFailed }

        keyDown.flags = .maskCommand
        keyUp.flags   = .maskCommand

        keyDown.post(tap: .cgAnnotatedSessionEventTap)
        keyUp.post(tap: .cgAnnotatedSessionEventTap)
    }
    #endif

    // MARK: - iOS implementation

    #if os(iOS)
    private static func pasteOnIOS(text: String) {
        UIPasteboard.general.string = text
        logger.info("Wrote \(text.count) chars to UIPasteboard (user must paste manually)")
        // Haptic feedback
        let generator = UINotificationFeedbackGenerator()
        generator.notificationOccurred(.success)
    }
    #endif
}

// MARK: - PasteError

public enum PasteError: Error, LocalizedError {
    case eventCreationFailed
    case accessibilityNotGranted

    public var errorDescription: String? {
        switch self {
        case .eventCreationFailed:
            return "Could not create paste keyboard event. Ensure Accessibility permission is granted."
        case .accessibilityNotGranted:
            return "Vox needs Accessibility permission to paste text. Please grant it in System Settings."
        }
    }
}
