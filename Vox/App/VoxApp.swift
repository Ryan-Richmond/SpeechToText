import SwiftUI
import SwiftData

@main
struct VoxApp: App {

    @State private var dictationCoordinator = DictationCoordinator.shared
    @State private var permissionsManager = PermissionsManager.shared
    @AppStorage("vox.onboardingComplete") private var onboardingComplete = false

    var body: some Scene {
        #if os(macOS)
        macOSScene
        #else
        iOSScene
        #endif
    }

    // MARK: - macOS: menu bar app

    #if os(macOS)
    var macOSScene: some Scene {
        MenuBarExtra("Vox", systemImage: menuBarIcon) {
            VoxMenuBarContent()
                .environment(dictationCoordinator)
        }
        .menuBarExtraStyle(.window)
        .modelContainer(for: modelTypes)
        .commands {
            CommandGroup(after: .appSettings) {
                Button("Settings…") {
                    openSettings()
                }
                .keyboardShortcut(",", modifiers: .command)
            }
        }
        .defaultSize(width: 320, height: 480)
        .onChange(of: dictationCoordinator.state) { _, _ in
            // Menu bar icon reacts to state
        }
    }

    private var menuBarIcon: String {
        switch dictationCoordinator.state {
        case .recording:  return "waveform.badge.mic"
        case .processing: return "ellipsis.circle"
        case .done:       return "checkmark.circle"
        case .error:      return "exclamationmark.circle"
        case .idle:       return "waveform"
        }
    }

    private func openSettings() {
        if let url = URL(string: "x-apple.systempreferences:") {
            NSWorkspace.shared.open(url)
        }
    }
    #endif

    // MARK: - iOS

    #if os(iOS)
    var iOSScene: some Scene {
        WindowGroup {
            ContentView()
                .environment(dictationCoordinator)
                .modelContainer(for: modelTypes)
                .sheet(isPresented: .constant(!onboardingComplete)) {
                    OnboardingFlow()
                        .environment(dictationCoordinator)
                }
        }
    }
    #endif

    // MARK: - SwiftData container

    private var modelTypes: [any PersistentModel.Type] {
        [Transcription.self, ModelConfig.self, DictionaryEntry.self, StyleProfile.self]
    }
}

// MARK: - macOS Menu Bar Content

#if os(macOS)
struct VoxMenuBarContent: View {
    @Environment(DictationCoordinator.self) private var coordinator
    @AppStorage("vox.onboardingComplete") private var onboardingComplete = false
    @State private var showSettings = false
    @State private var showHistory = false
    @State private var showOnboarding = false

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: "waveform")
                    .foregroundStyle(Color.voxPurple)
                    .font(.title2)
                Text("Vox")
                    .font(.voxHeadline)
                Spacer()
                Circle()
                    .fill(statusColor)
                    .frame(width: 8, height: 8)
            }
            .padding(Spacing.md)

            Divider()

            // State display
            stateContent
                .frame(maxWidth: .infinity, minHeight: 80)
                .padding(Spacing.md)

            Divider()

            // Action buttons
            VStack(spacing: Spacing.xs) {
                menuButton("History", icon: "clock") { showHistory = true }
                menuButton("Settings", icon: "gear")  { showSettings = true }
                if !onboardingComplete {
                    menuButton("Setup", icon: "arrow.right.circle") { showOnboarding = true }
                }
                Divider()
                menuButton("Quit Vox", icon: "power", role: .destructive) {
                    NSApplication.shared.terminate(nil)
                }
            }
            .padding(Spacing.sm)
        }
        .frame(width: 300)
        .sheet(isPresented: $showSettings) { SettingsView() }
        .sheet(isPresented: $showHistory) { HistoryView() }
        .sheet(isPresented: $showOnboarding) {
            OnboardingFlow().environment(coordinator)
        }
        .task {
            // Register hotkeys
            HotkeyManager.shared.register()
            CommandModeCoordinator.shared.registerHotkey()

            // Load models if onboarding is complete
            if onboardingComplete {
                await loadModelsIfNeeded()
            }
        }
    }

    @ViewBuilder
    private var stateContent: some View {
        switch coordinator.state {
        case .idle:
            VStack(spacing: Spacing.sm) {
                Image(systemName: "mic.badge.plus")
                    .font(.system(size: 32))
                    .foregroundStyle(Color.voxPurple.opacity(0.7))
                Text("Double-tap Fn to dictate")
                    .font(.voxCaption)
                    .foregroundStyle(.secondary)
            }
        case .recording:
            VStack(spacing: Spacing.sm) {
                WaveformView(rmsLevel: coordinator.rmsLevel, barCount: 13)
                Text("Listening…")
                    .font(.voxBody)
                    .foregroundStyle(Color.voxRecording)
            }
        case .processing:
            VStack(spacing: Spacing.sm) {
                ProgressView()
                Text("Processing…")
                    .font(.voxBody)
                    .foregroundStyle(.secondary)
                    .shimmer()
            }
        case .done(let text):
            VStack(alignment: .leading, spacing: Spacing.xs) {
                Label("Inserted", systemImage: "checkmark.circle.fill")
                    .foregroundStyle(Color.voxDone)
                    .font(.voxCaption)
                Text(text.prefix(120))
                    .font(.voxCaption)
                    .foregroundStyle(.secondary)
                    .lineLimit(3)
            }
        case .error(let msg):
            Label(msg, systemImage: "exclamationmark.circle.fill")
                .foregroundStyle(Color.voxRecording)
                .font(.voxCaption)
                .multilineTextAlignment(.center)
        }
    }

    private var statusColor: Color {
        switch coordinator.state {
        case .idle:       return .secondary.opacity(0.5)
        case .recording:  return Color.voxRecording
        case .processing: return Color.voxProcessing
        case .done:       return Color.voxDone
        case .error:      return Color.voxRecording
        }
    }

    private func menuButton(
        _ title: String,
        icon: String,
        role: ButtonRole? = nil,
        action: @escaping () -> Void
    ) -> some View {
        Button(role: role, action: action) {
            Label(title, systemImage: icon)
                .frame(maxWidth: .infinity, alignment: .leading)
                .font(.voxBody)
        }
        .buttonStyle(.plain)
        .padding(.horizontal, Spacing.sm)
        .padding(.vertical, 6)
        .contentShape(Rectangle())
    }

    private func loadModelsIfNeeded() async {
        let capability = DeviceCapabilityService.shared
        let catalog = ModelConfig.defaultCatalog()
        guard let whisper = capability.recommendedWhisperModel(from: catalog),
              let gemma   = capability.recommendedGemmaModel(from: catalog),
              whisper.isDownloaded, gemma.isDownloaded else { return }
        try? await PipelineActor.shared.loadModels(whisper: whisper, gemma: gemma)
    }
}
#endif
