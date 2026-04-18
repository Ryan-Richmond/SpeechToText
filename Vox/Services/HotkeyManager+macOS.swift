#if os(macOS)
import AppKit
import Carbon.HIToolbox
import OSLog

@MainActor
final class HotkeyManager: ObservableObject {

    @Published private(set) var isListening = false

    /// Fires when the user activates the dictation hotkey.
    var onActivate: (() -> Void)?
    /// Fires when the user releases the dictation hotkey.
    var onDeactivate: (() -> Void)?

    private var flagsMonitor: Any?
    private var fnPressCount = 0
    private var lastFnPressTime: Date?

    /// Start listening for double-tap Fn (the default global hotkey).
    func start() {
        guard !isListening else { return }

        flagsMonitor = NSEvent.addGlobalMonitorForEvents(matching: .flagsChanged) { [weak self] event in
            Task { @MainActor in
                self?.handleFlagsChanged(event)
            }
        }

        isListening = true
        Log.hotkey.info("Hotkey manager started (double-tap Fn)")
    }

    func stop() {
        if let monitor = flagsMonitor {
            NSEvent.removeMonitor(monitor)
            flagsMonitor = nil
        }
        isListening = false
        fnPressCount = 0
        Log.hotkey.info("Hotkey manager stopped")
    }

    // MARK: - Private

    private func handleFlagsChanged(_ event: NSEvent) {
        let fnPressed = event.modifierFlags.contains(.function)

        guard fnPressed else {
            fnPressCount = 0
            return
        }

        let now = Date()
        if let last = lastFnPressTime, now.timeIntervalSince(last) < 0.4 {
            fnPressCount += 1
        } else {
            fnPressCount = 1
        }
        lastFnPressTime = now

        if fnPressCount >= 2 {
            fnPressCount = 0
            Log.hotkey.info("Double-Fn detected — activating dictation")
            onActivate?()
        }
    }
}
#endif
