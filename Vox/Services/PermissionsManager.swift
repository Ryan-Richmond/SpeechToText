import Foundation
import AVFoundation
import os
#if os(macOS)
import ApplicationServices
import AppKit
#endif

// MARK: - PermissionsManager

/// Checks and requests all permissions Vox needs.
/// All methods are safe to call from any context.
@MainActor
public final class PermissionsManager: ObservableObject {

    public static let shared = PermissionsManager()
    private init() {}

    private let logger = Logger.vox(.permissions)

    // MARK: - Published state

    @Published public var microphoneGranted: Bool = false
    @Published public var accessibilityGranted: Bool = false

    // MARK: - Check (non-prompting)

    public func refreshStatus() {
        microphoneGranted = checkMicrophoneStatus()
        accessibilityGranted = checkAccessibilityStatus()
    }

    public func checkMicrophoneStatus() -> Bool {
        #if os(macOS)
        return AVCaptureDevice.authorizationStatus(for: .audio) == .authorized
        #else
        return AVAudioApplication.shared.recordPermission == .granted
        #endif
    }

    public func checkAccessibilityStatus() -> Bool {
        #if os(macOS)
        return AXIsProcessTrusted()
        #else
        return true  // Not applicable on iOS
        #endif
    }

    // MARK: - Request

    public func requestMicrophonePermission() async -> Bool {
        #if os(macOS)
        let status = AVCaptureDevice.authorizationStatus(for: .audio)
        if status == .authorized { 
            microphoneGranted = true
            return true 
        }
        let granted = await AVCaptureDevice.requestAccess(for: .audio)
        microphoneGranted = granted
        logger.info("Microphone permission: \(granted ? "granted" : "denied")")
        return granted
        #else
        let granted = await AVAudioApplication.requestRecordPermission()
        microphoneGranted = granted
        return granted
        #endif
    }

    #if os(macOS)
    /// Opens System Settings → Privacy & Security → Accessibility.
    /// The user must manually grant access; we cannot prompt programmatically.
    public func openAccessibilitySettings() {
        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility")!
        NSWorkspace.shared.open(url)
        logger.info("Opened Accessibility settings")
    }

    /// Opens System Settings → Privacy & Security → Microphone.
    public func openMicrophoneSettings() {
        let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone")!
        NSWorkspace.shared.open(url)
    }
    #endif
}
