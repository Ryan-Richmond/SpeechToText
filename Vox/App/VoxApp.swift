import SwiftUI

@main
struct VoxApp: App {

    var body: some Scene {
        #if os(macOS)
        MenuBarExtra("Vox", systemImage: "mic.circle") {
            ContentView()
                .frame(width: 320, height: 240)
        }
        .menuBarExtraStyle(.window)
        #else
        WindowGroup {
            ContentView()
        }
        #endif
    }
}
