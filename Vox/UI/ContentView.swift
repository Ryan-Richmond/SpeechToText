import SwiftUI

// MARK: - ContentView
// iOS main view + debug/test interface for both platforms.

struct ContentView: View {
    @Environment(DictationCoordinator.self) private var coordinator

    var body: some View {
        NavigationStack {
            VStack(spacing: Spacing.lg) {
                // App icon / header
                VStack(spacing: Spacing.sm) {
                    Image(systemName: "waveform")
                        .font(.system(size: 52))
                        .foregroundStyle(Color.voxPurple)
                        .symbolEffect(.variableColor.reversing, value: coordinator.state == .recording)

                    Text("Vox")
                        .font(.voxTitle)

                    Text(stateDescription)
                        .font(.voxBody)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                        .animation(.easeInOut, value: coordinator.state)
                }

                // Waveform (visible while recording)
                if case .recording = coordinator.state {
                    WaveformView(rmsLevel: coordinator.rmsLevel, barCount: 13)
                        .frame(height: 40)
                        .transition(.opacity.combined(with: .scale))
                }

                // Dictation result
                if case .done(let text) = coordinator.state {
                    Text(text)
                        .font(.voxBody)
                        .padding(Spacing.md)
                        .background(Color.primary.opacity(0.05))
                        .clipShape(RoundedRectangle(cornerRadius: CornerRadius.card))
                        .transition(.opacity.combined(with: .move(edge: .bottom)))
                }

                // Error
                if case .error(let msg) = coordinator.state {
                    Label(msg, systemImage: "exclamationmark.triangle")
                        .font(.voxCaption)
                        .foregroundStyle(Color.voxRecording)
                        .multilineTextAlignment(.center)
                }

                // iOS dictation button (macOS uses hotkey)
                #if os(iOS)
                dictationButton
                #endif

                Spacer()

                // Navigation
                HStack(spacing: Spacing.lg) {
                    NavigationLink(destination: HistoryView()) {
                        Label("History", systemImage: "clock")
                    }
                    NavigationLink(destination: SettingsView()) {
                        Label("Settings", systemImage: "gear")
                    }
                }
                .font(.voxBody)
            }
            .padding(Spacing.lg)
            .navigationTitle("Vox")
            #if os(iOS)
            .navigationBarTitleDisplayMode(.inline)
            #endif
            .animation(.spring(response: 0.4, dampingFraction: 0.75), value: coordinator.state)
        }
    }

    // MARK: - iOS dictation button

    #if os(iOS)
    private var dictationButton: some View {
        Button {
            switch coordinator.state {
            case .idle:      coordinator.beginRecording()
            case .recording: coordinator.endRecordingAndProcess()
            default:         break
            }
        } label: {
            Circle()
                .fill(buttonColor)
                .frame(width: 80, height: 80)
                .overlay(
                    Image(systemName: coordinator.state == .recording ? "stop.fill" : "mic.fill")
                        .font(.system(size: 28, weight: .semibold))
                        .foregroundStyle(.white)
                )
                .shadow(color: buttonColor.opacity(0.4), radius: 12, x: 0, y: 4)
        }
        .buttonStyle(.plain)
        .scaleEffect(coordinator.state == .recording ? 1.08 : 1.0)
        .animation(.spring(response: 0.3), value: coordinator.state == .recording)
        .disabled(coordinator.state == .processing)
    }

    private var buttonColor: Color {
        switch coordinator.state {
        case .recording:  return Color.voxRecording
        case .processing: return Color.secondary
        default:          return Color.voxPurple
        }
    }
    #endif

    // MARK: - State description

    private var stateDescription: String {
        switch coordinator.state {
        case .idle:
            #if os(macOS)
            return "Double-tap Fn to dictate"
            #else
            return "Tap the button below to start dictating"
            #endif
        case .recording:   return "Listening… speak naturally"
        case .processing:  return "Cleaning up your text…"
        case .done:        return "Done! Text pasted at cursor."
        case .error:       return "Something went wrong."
        }
    }
}
