import Foundation
import os
#if os(macOS)
import AppKit

// MARK: - CommandModeCoordinator

/// Handles the Command Mode flow:
/// User selects text → presses ⌃⌘D → speaks an instruction → LLM rewrites selection.
@MainActor
public final class CommandModeCoordinator {

    public static let shared = CommandModeCoordinator()
    private init() {}

    private let logger = Logger.vox(.pipeline)
    private let audioCapture = AudioCapture()

    // MARK: - State

    public enum CommandState: Equatable {
        case idle
        case listeningForInstruction
        case processing
        case done(String)
        case error(String)
    }

    public private(set) var state: CommandState = .idle

    // MARK: - Hotkey registration

    private var commandMonitor: Any?

    public func registerHotkey() {
        guard commandMonitor == nil else { return }
        // ⌃⌘D (keyCode 2 = d)
        commandMonitor = NSEvent.addGlobalMonitorForEvents(matching: .keyDown) { [weak self] event in
            guard event.keyCode == 2,
                  event.modifierFlags.contains([.command, .control]) else { return }
            Task { @MainActor in await self?.trigger() }
        }
        logger.info("CommandModeCoordinator: registered ⌃⌘D hotkey")
    }

    // MARK: - Trigger

    public func trigger() async {
        guard case .idle = state else { return }

        // Capture selected text via copy simulation
        let selection = captureSelection()
        guard !selection.isEmpty else {
            logger.warning("Command Mode triggered with no selection")
            return
        }

        state = .listeningForInstruction
        logger.info("Command Mode: listening for instruction (selection: \"\(selection.prefix(40))\")")

        do {
            try await audioCapture.startRecording()

            // Record for up to 10 seconds or until user releases key (MVP: fixed duration)
            try await Task.sleep(nanoseconds: 8_000_000_000)
            let audio = await audioCapture.stopRecording()

            state = .processing
            let context = AppContextService.currentContext()
            let result = try await PipelineActor.shared.command(
                selection: selection,
                audio: audio,
                context: context
            )

            try await PasteService.paste(text: result.cleanedText)
            state = .done(result.cleanedText)
            logger.info("Command Mode: done in \(result.latencyMs)ms")

            try await Task.sleep(nanoseconds: 2_000_000_000)
            state = .idle

        } catch {
            state = .error(error.localizedDescription)
            logger.error("Command Mode error: \(error)")
            try? await Task.sleep(nanoseconds: 3_000_000_000)
            state = .idle
        }
    }

    // MARK: - Selection capture

    private func captureSelection() -> String {
        let pasteboard = NSPasteboard.general
        let saved = pasteboard.string(forType: .string)

        // Simulate Cmd+C to copy current selection
        let src = CGEventSource(stateID: .hidSystemState)
        if let keyDown = CGEvent(keyboardEventSource: src, virtualKey: 0x08, keyDown: true),
           let keyUp   = CGEvent(keyboardEventSource: src, virtualKey: 0x08, keyDown: false) {
            keyDown.flags = .maskCommand
            keyUp.flags   = .maskCommand
            keyDown.post(tap: .cgAnnotatedSessionEventTap)
            keyUp.post(tap: .cgAnnotatedSessionEventTap)
        }

        // Brief pause for pasteboard to update
        Thread.sleep(forTimeInterval: 0.15)
        let selection = NSPasteboard.general.string(forType: .string) ?? ""

        // Restore previous pasteboard if selection was empty or same
        if selection.isEmpty, let saved {
            pasteboard.clearContents()
            pasteboard.setString(saved, forType: .string)
        }

        return selection
    }
}
#endif
