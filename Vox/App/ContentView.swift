import SwiftUI

/// Sprint 0 placeholder. Sprint 1 replaces this with `DebugDictationView`.
struct ContentView: View {

    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "waveform")
                .font(.system(size: 48))
                .foregroundStyle(.tint)

            Text("Vox")
                .font(.title2.weight(.semibold))

            Text("Pre-MVP build — Sprint 0")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding()
    }
}

#Preview {
    ContentView()
}
