import SwiftUI

// MARK: - DictationOverlay

/// Floating pill overlay showing dictation state.
/// On macOS this is displayed as a non-activating NSPanel that floats above all windows.
/// On iOS it's an overlay on the main window.
public struct DictationOverlay: View {

    @Environment(DictationCoordinator.self) private var coordinator

    public var body: some View {
        pillContent
            .padding(.horizontal, Spacing.md)
            .padding(.vertical, Spacing.sm)
            .background(pillBackground)
            .clipShape(Capsule())
            .shadow(color: .black.opacity(0.25), radius: 12, x: 0, y: 4)
            .scaleEffect(isVisible ? 1.0 : 0.85)
            .opacity(isVisible ? 1.0 : 0.0)
            .animation(.spring(response: 0.3, dampingFraction: 0.7), value: coordinator.state)
    }

    // MARK: - Private

    private var isVisible: Bool {
        switch coordinator.state {
        case .idle: return false
        default:    return true
        }
    }

    @ViewBuilder
    private var pillContent: some View {
        HStack(spacing: Spacing.sm) {
            stateIcon
            stateLabel
            if case .recording = coordinator.state {
                WaveformView(rmsLevel: coordinator.rmsLevel)
            }
        }
    }

    @ViewBuilder
    private var stateIcon: some View {
        switch coordinator.state {
        case .recording:
            Circle()
                .fill(Color.voxRecording)
                .frame(width: 8, height: 8)
                .overlay(
                    Circle()
                        .stroke(Color.voxRecording.opacity(0.4), lineWidth: 4)
                        .scaleEffect(1.5)
                        .animation(.easeOut(duration: 0.8).repeatForever(), value: coordinator.rmsLevel)
                )
        case .processing:
            ProgressView()
                .scaleEffect(0.7)
                .tint(.white)
        case .done:
            Image(systemName: "checkmark.circle.fill")
                .foregroundStyle(Color.voxDone)
        case .error:
            Image(systemName: "exclamationmark.circle.fill")
                .foregroundStyle(Color.voxRecording)
        case .idle:
            EmptyView()
        }
    }

    @ViewBuilder
    private var stateLabel: some View {
        switch coordinator.state {
        case .idle:
            EmptyView()
        case .recording:
            Text("Listening…")
                .font(.voxBody)
                .foregroundStyle(.white)
        case .processing:
            Text("Processing…")
                .font(.voxBody)
                .foregroundStyle(.white)
                .shimmer()
        case .done(let text):
            Text(text.prefix(40) + (text.count > 40 ? "…" : ""))
                .font(.voxCaption)
                .foregroundStyle(.white.opacity(0.85))
                .lineLimit(1)
        case .error(let msg):
            Text(msg)
                .font(.voxCaption)
                .foregroundStyle(.white.opacity(0.9))
                .lineLimit(2)
        }
    }

    private var pillBackground: some View {
        Group {
            switch coordinator.state {
            case .recording:
                LinearGradient(
                    colors: [Color.voxRecording, Color.voxRecording.opacity(0.85)],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
            case .processing:
                LinearGradient(
                    colors: [Color.voxPurpleDark, Color.voxBlue.opacity(0.9)],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
            case .done:
                Color.black.opacity(0.8)
            case .error:
                Color.black.opacity(0.8)
            case .idle:
                Color.clear
            }
        }
    }
}
