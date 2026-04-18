import SwiftUI

struct DebugDictationView: View {

    @StateObject private var coordinator = DictationCoordinator()

    var body: some View {
        VStack(spacing: 20) {
            statusView

            actionButton

            if !coordinator.lastRawTranscript.isEmpty {
                transcriptSection
            }

            Spacer()
        }
        .padding(24)
        .frame(minWidth: 360, minHeight: 400)
        .task {
            await coordinator.setup()
        }
    }

    // MARK: - Subviews

    @ViewBuilder
    private var statusView: some View {
        switch coordinator.state {
        case .idle:
            Label("Ready", systemImage: "mic")
                .foregroundStyle(.secondary)
        case .recording:
            Label("Recording...", systemImage: "waveform")
                .foregroundStyle(.red)
                .symbolEffect(.pulse)
        case .processing:
            Label("Transcribing...", systemImage: "brain")
                .foregroundStyle(.orange)
                .symbolEffect(.pulse)
        case .result:
            Label("Done — \(coordinator.latencyMs)ms", systemImage: "checkmark.circle")
                .foregroundStyle(.green)
        case .error(let message):
            Label(message, systemImage: "exclamationmark.triangle")
                .foregroundStyle(.red)
        }
    }

    @ViewBuilder
    private var actionButton: some View {
        switch coordinator.state {
        case .recording:
            Button("Stop") {
                Task { await coordinator.stopDictation() }
            }
            .buttonStyle(.borderedProminent)
            .tint(.red)
            .keyboardShortcut(.return)
        case .processing:
            ProgressView()
        default:
            Button("Dictate") {
                Task { await coordinator.startDictation() }
            }
            .buttonStyle(.borderedProminent)
            .keyboardShortcut(.return)
        }
    }

    @ViewBuilder
    private var transcriptSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Raw transcript")
                .font(.caption.weight(.semibold))
                .foregroundStyle(.secondary)

            Text(coordinator.lastRawTranscript)
                .textSelection(.enabled)
                .font(.body)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(8)
                .background(.quaternary, in: .rect(cornerRadius: 8))

            if coordinator.lastTranscript != coordinator.lastRawTranscript {
                Text("Cleaned")
                    .font(.caption.weight(.semibold))
                    .foregroundStyle(.secondary)

                Text(coordinator.lastTranscript)
                    .textSelection(.enabled)
                    .font(.body)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(8)
                    .background(.quaternary, in: .rect(cornerRadius: 8))
            }

            HStack {
                Text("Latency: \(coordinator.latencyMs)ms")
                Spacer()
                Text("Tier: \(DeviceCapabilityService.detectTier().rawValue)")
                Spacer()
                Text("RAM: \(DeviceCapabilityService.totalRAMDescription())")
            }
            .font(.caption2)
            .foregroundStyle(.tertiary)
        }
    }
}

#Preview {
    DebugDictationView()
}
