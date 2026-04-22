import SwiftUI
import SwiftData

// MARK: - OnboardingFlow

/// Multi-step first-launch flow:
/// 1. Mic permission
/// 2. Accessibility permission (macOS)
/// 3. Model download
/// 4. First dictation demo
public struct OnboardingFlow: View {

    @State private var step: OnboardingStep = .welcome
    @State private var downloadProgress: ModelDownloadService.DownloadProgress?
    @State private var downloadError: String?
    @State private var isDownloading = false

    @Environment(\.dismiss) private var dismiss
    @Environment(DictationCoordinator.self) private var coordinator

    public var body: some View {
        VStack(spacing: 0) {
            // Step indicator
            HStack(spacing: Spacing.sm) {
                ForEach(OnboardingStep.allCases, id: \.self) { s in
                    Circle()
                        .fill(s <= step ? Color.voxPurple : Color.primary.opacity(0.15))
                        .frame(width: 8, height: 8)
                        .animation(.easeInOut, value: step)
                }
            }
            .padding(.top, Spacing.lg)

            Spacer()

            stepContent
                .transition(.asymmetric(
                    insertion: .move(edge: .trailing).combined(with: .opacity),
                    removal: .move(edge: .leading).combined(with: .opacity)
                ))
                .animation(.easeInOut(duration: 0.3), value: step)

            Spacer()

            actionButton
                .padding(.bottom, Spacing.xl)
        }
        .padding(.horizontal, Spacing.lg)
        .frame(minWidth: 420, minHeight: 480)
    }

    // MARK: - Step content

    @ViewBuilder
    private var stepContent: some View {
        switch step {
        case .welcome:
            OnboardingStepView(
                icon: "waveform.badge.mic",
                iconColor: .voxPurple,
                title: "Welcome to Vox",
                description: "Press a hotkey, speak, and polished text appears at your cursor — instantly, on your device, with zero network traffic."
            )
        case .microphone:
            OnboardingStepView(
                icon: "mic.fill",
                iconColor: .voxBlue,
                title: "Microphone Access",
                description: "Vox needs your microphone to transcribe speech. Audio is processed entirely on your device and never stored."
            )
        case .accessibility:
            OnboardingStepView(
                icon: "accessibility",
                iconColor: .voxPurple,
                title: "Accessibility Permission",
                description: "To paste dictated text into any app with a hotkey, Vox needs Accessibility access.\n\nClick 'Open Settings', find Vox in the list, and toggle it on."
            )
        case .download:
            downloadStepView
        case .ready:
            OnboardingStepView(
                icon: "checkmark.seal.fill",
                iconColor: .voxDone,
                title: "You're all set!",
                description: "Double-tap Fn to start dictating. Speak naturally — Vox will clean up filler words and punctuation automatically."
            )
        }
    }

    @ViewBuilder
    private var downloadStepView: some View {
        VStack(spacing: Spacing.md) {
            Image(systemName: "cpu")
                .font(.system(size: 52))
                .foregroundStyle(Color.voxPurple)

            Text("Downloading AI Models")
                .font(.voxTitle)

            Text("Whisper (speech recognition) and Gemma 4 (text cleanup) will download once and stay on your device. You'll need ~6 GB of space and a few minutes.")
                .font(.voxBody)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)

            if let progress = downloadProgress {
                VStack(spacing: Spacing.xs) {
                    ProgressView(value: progress.fractionCompleted)
                        .tint(Color.voxPurple)
                    HStack {
                        Text(progress.modelName)
                            .font(.voxCaption)
                            .foregroundStyle(.secondary)
                        Spacer()
                        Text(String(format: "%.0f%%", progress.fractionCompleted * 100))
                            .font(.voxCaption.monospacedDigit())
                    }
                }
                .padding(.top, Spacing.sm)
            }

            if let error = downloadError {
                Text(error)
                    .font(.voxCaption)
                    .foregroundStyle(Color.voxRecording)
                    .multilineTextAlignment(.center)
            }
        }
    }

    // MARK: - Action button

    @ViewBuilder
    private var actionButton: some View {
        Button(action: handleAction) {
            Text(buttonTitle)
                .font(.voxHeadline)
                .frame(maxWidth: .infinity)
                .padding(.vertical, Spacing.sm)
        }
        .buttonStyle(.borderedProminent)
        .tint(Color.voxPurple)
        .disabled(isDownloading && step == .download)
        .controlSize(.large)
    }

    private var buttonTitle: String {
        switch step {
        case .welcome:       return "Get Started"
        case .microphone:    return "Grant Microphone Access"
        case .accessibility: return "Open Settings"
        case .download:      return isDownloading ? "Downloading…" : "Download Models"
        case .ready:         return "Start Using Vox"
        }
    }

    // MARK: - Action handling

    private func handleAction() {
        switch step {
        case .welcome:
            advance()

        case .microphone:
            Task {
                let granted = await PermissionsManager.shared.requestMicrophonePermission()
                if granted { advance() }
            }

        case .accessibility:
            #if os(macOS)
            PermissionsManager.shared.openAccessibilitySettings()
            #endif
            // Poll for permission — user will toggle it in Settings
            Task {
                for _ in 0..<20 {
                    try? await Task.sleep(nanoseconds: 500_000_000)
                    if PermissionsManager.shared.checkAccessibilityStatus() {
                        advance()
                        return
                    }
                }
                // Let them proceed even if they skip
                advance()
            }

        case .download:
            Task { await startDownloads() }

        case .ready:
            UserDefaults.standard.set(true, forKey: "vox.onboardingComplete")
            dismiss()
        }
    }

    private func advance() {
        withAnimation {
            step = step.next ?? step
        }
    }

    // MARK: - Download

    private func startDownloads() async {
        isDownloading = true
        downloadError = nil

        let capability = DeviceCapabilityService.shared
        let catalog = ModelConfig.defaultCatalog()

        guard let whisperModel = capability.recommendedWhisperModel(from: catalog),
              let gemmaModel   = capability.recommendedGemmaModel(from: catalog) else {
            downloadError = "Your device doesn't meet the minimum requirements (8 GB RAM)."
            isDownloading = false
            return
        }

        // Download Whisper
        do {
            for try await progress in await ModelDownloadService.shared.download(model: whisperModel) {
                await MainActor.run { downloadProgress = progress }
            }
        } catch {
            await MainActor.run {
                downloadError = error.localizedDescription
                isDownloading = false
            }
            return
        }

        // Download Gemma
        do {
            for try await progress in await ModelDownloadService.shared.download(model: gemmaModel) {
                await MainActor.run { downloadProgress = progress }
            }
        } catch {
            await MainActor.run {
                downloadError = error.localizedDescription
                isDownloading = false
            }
            return
        }

        // Load pipeline
        do {
            try await PipelineActor.shared.loadModels(whisper: whisperModel, gemma: gemmaModel)
        } catch {
            await MainActor.run {
                downloadError = "Models downloaded but failed to load: \(error.localizedDescription)"
                isDownloading = false
            }
            return
        }

        await MainActor.run {
            isDownloading = false
            advance()
        }
    }
}

// MARK: - OnboardingStep

private enum OnboardingStep: Int, CaseIterable, Comparable {
    case welcome, microphone, accessibility, download, ready

    var next: OnboardingStep? {
        OnboardingStep(rawValue: rawValue + 1)
    }

    static func < (lhs: OnboardingStep, rhs: OnboardingStep) -> Bool {
        lhs.rawValue < rhs.rawValue
    }
}

// MARK: - OnboardingStepView

private struct OnboardingStepView: View {
    let icon: String
    let iconColor: Color
    let title: String
    let description: String

    var body: some View {
        VStack(spacing: Spacing.md) {
            Image(systemName: icon)
                .font(.system(size: 56))
                .foregroundStyle(iconColor)
                .symbolEffect(.bounce, value: icon)

            Text(title)
                .font(.voxTitle)
                .multilineTextAlignment(.center)

            Text(description)
                .font(.voxBody)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
        }
    }
}
