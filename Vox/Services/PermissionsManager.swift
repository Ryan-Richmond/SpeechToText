import AVFoundation
import OSLog

#if os(macOS)
import AppKit
#elseif os(iOS)
import UIKit
#endif

@MainActor
final class PermissionsManager: ObservableObject {

    @Published private(set) var microphoneGranted = false
    @Published private(set) var accessibilityGranted = false

    func checkAll() async {
        microphoneGranted = await checkMicrophone()
        #if os(macOS)
        accessibilityGranted = checkAccessibility()
        #else
        accessibilityGranted = true
        #endif
    }

    // MARK: - Microphone

    func requestMicrophone() async -> Bool {
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        switch status {
        case .authorized:
            microphoneGranted = true
            return true
        case .notDetermined:
            let granted = await AVCaptureDevice.requestAccess(for: .audio)
            microphoneGranted = granted
            return granted
        default:
            microphoneGranted = false
            Log.permissions.warning("Microphone permission denied. Opening Settings.")
            openSettings()
            return false
        }
    }

    // MARK: - Accessibility (macOS only)

    #if os(macOS)
    func checkAccessibility() -> Bool {
        let trusted = AXIsProcessTrusted()
        accessibilityGranted = trusted
        if !trusted {
            Log.permissions.warning("Accessibility permission not granted.")
        }
        return trusted
    }

    func promptAccessibility() {
        let options = [kAXTrustedCheckOptionPrompt.takeRetainedValue(): true] as CFDictionary
        AXIsProcessTrustedWithOptions(options)
    }
    #endif

    // MARK: - Private

    private func checkMicrophone() async -> Bool {
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        return status == .authorized
    }

    private func openSettings() {
        #if os(macOS)
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone") {
            NSWorkspace.shared.open(url)
        }
        #else
        if let url = URL(string: UIApplication.openSettingsURLString) {
            UIApplication.shared.open(url)
        }
        #endif
    }
}
