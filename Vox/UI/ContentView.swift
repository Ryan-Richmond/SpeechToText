import SwiftUI

struct ContentView: View {
    var body: some View {
        VStack(spacing: 12) {
            Image(systemName: "waveform")
                .font(.system(size: 42))
            Text("Vox")
                .font(.title)
            Text("Sprint 0 bootstrap build")
                .font(.subheadline)
                .foregroundStyle(.secondary)
        }
        .padding(24)
    }
}

#Preview {
    ContentView()
}
