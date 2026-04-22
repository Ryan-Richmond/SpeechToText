import Foundation
import os
#if os(macOS)
import AppKit
import Carbon

// MARK: - HotkeyManager

/// Registers a global hotkey that triggers dictation from any app.
/// Default: double-tap Fn key.
/// Uses NSEvent global monitor as primary approach; Carbon fallback optional.
///
/// Accessibility permission required for global monitoring.
@MainActor
public final class HotkeyManager {

    public static let shared = HotkeyManager()
    private init() {}

    private let logger = Logger.vox(.app)
    private var globalMonitor: Any?
    private var lastFnTapTime: Date?
    private let doubleTapInterval: TimeInterval = 0.4

    // MARK: - Registration

    public func register() {
        guard globalMonitor == nil else { return }

        // Monitor all key events globally
        globalMonitor = NSEvent.addGlobalMonitorForEvents(matching: .keyDown) { [weak self] event in
            self?.handleKeyEvent(event)
        }

        // Also monitor Fn key via flags changed
        NSEvent.addGlobalMonitorForEvents(matching: .flagsChanged) { [weak self] event in
            self?.handleFlagsChanged(event)
        }

        logger.info("HotkeyManager: registered global monitor")
    }

    public func unregister() {
        if let monitor = globalMonitor {
            NSEvent.removeMonitor(monitor)
            globalMonitor = nil
        }
    }

    // MARK: - Event handling

    private func handleFlagsChanged(_ event: NSEvent) {
        // Fn key = .function in flags
        let isFnDown = event.modifierFlags.contains(.function)
        guard !isFnDown else { return } // Only react on Fn release

        let now = Date()
        if let last = lastFnTapTime, now.timeIntervalSince(last) < doubleTapInterval {
            // Double-tap detected
            lastFnTapTime = nil
            Task { @MainActor in
                self.onHotkeyTriggered()
            }
        } else {
            lastFnTapTime = now
        }
    }

    private func handleKeyEvent(_ event: NSEvent) {
        // Future: configurable hotkeys (e.g. ⌃⌘D for Command Mode)
    }

    // MARK: - Hotkey action

    private func onHotkeyTriggered() {
        let coordinator = DictationCoordinator.shared
        switch coordinator.state {
        case .idle:
            coordinator.beginRecording()
        case .recording:
            coordinator.endRecordingAndProcess()
        default:
            break
        }
    }
}
#endif
