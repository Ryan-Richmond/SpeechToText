import SwiftUI

@main
struct VoxApp: App {

    #if os(macOS)
    @StateObject private var hotkeyManager = HotkeyManager()
    #endif

    var body: some Scene {
        #if os(macOS)
        MenuBarExtra("Vox", systemImage: "mic.circle") {
            DebugDictationView()
                .frame(width: 400, height: 480)
        }
        .menuBarExtraStyle(.window)
        #else
        WindowGroup {
            DebugDictationView()
        }
        #endif
    }
}
