import SwiftUI

// MARK: - SettingsView

public struct SettingsView: View {
    public var body: some View {
        TabView {
            GeneralSettings()
                .tabItem { Label("General", systemImage: "gear") }

            ModelSettings()
                .tabItem { Label("Models", systemImage: "cpu") }

            PrivacySettings()
                .tabItem { Label("Privacy", systemImage: "lock.shield") }
        }
        #if os(macOS)
        .frame(width: 520, height: 380)
        #endif
    }
}

// MARK: - GeneralSettings

public struct GeneralSettings: View {
    @AppStorage("vox.hotkey.description") private var hotkeyDescription = "Double-tap Fn"
    @AppStorage("vox.launchAtLogin") private var launchAtLogin = false

    public var body: some View {
        Form {
            Section("Hotkey") {
                HStack {
                    Text("Dictation trigger")
                    Spacer()
                    Text(hotkeyDescription)
                        .foregroundStyle(.secondary)
                        .padding(.horizontal, Spacing.sm)
                        .padding(.vertical, Spacing.xs)
                        .background(Color.primary.opacity(0.06))
                        .clipShape(RoundedRectangle(cornerRadius: CornerRadius.small))
                }
                Text("Double-tap Fn to start, tap again to stop and paste.")
                    .font(.voxCaption)
                    .foregroundStyle(.tertiary)
            }

            #if os(macOS)
            Section("Startup") {
                Toggle("Launch at login", isOn: $launchAtLogin)
            }
            #endif
        }
        .navigationTitle("General")
        .formStyle(.grouped)
    }
}

// MARK: - ModelSettings

public struct ModelSettings: View {
    @State private var catalog = ModelConfig.defaultCatalog()

    public var body: some View {
        Form {
            Section("Installed Models") {
                ForEach(catalog, id: \.localFilename) { model in
                    HStack {
                        VStack(alignment: .leading, spacing: 2) {
                            Text(model.name)
                                .font(.voxBody)
                            Text(model.modelType.rawValue.capitalized + " · " + model.quantization)
                                .font(.voxCaption)
                                .foregroundStyle(.secondary)
                        }
                        Spacer()
                        if model.isDownloaded {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundStyle(Color.voxDone)
                        } else {
                            Image(systemName: "arrow.down.circle")
                                .foregroundStyle(Color.voxPurple)
                        }
                    }
                }
            }

            Section {
                HStack {
                    Text("Recommended tier")
                    Spacer()
                    Text(DeviceCapabilityService.shared.tier.rawValue.capitalized)
                        .foregroundStyle(.secondary)
                }
                HStack {
                    Text("Physical RAM")
                    Spacer()
                    Text(String(format: "%.0f GB", DeviceCapabilityService.shared.physicalMemoryGB))
                        .monospacedDigit()
                        .foregroundStyle(.secondary)
                }
            } header: {
                Text("Device Info")
            }
        }
        .navigationTitle("Models")
        .formStyle(.grouped)
    }
}

// MARK: - PrivacySettings

public struct PrivacySettings: View {
    @Environment(\.modelContext) private var modelContext

    public var body: some View {
        Form {
            Section("Data") {
                Button("Clear Transcription History", role: .destructive) {
                    // Delete all Transcription records
                    try? modelContext.delete(model: Transcription.self)
                }
            }

            Section("Permissions") {
                #if os(macOS)
                HStack {
                    Text("Microphone")
                    Spacer()
                    Text(PermissionsManager.shared.microphoneGranted ? "Granted" : "Denied")
                        .foregroundStyle(PermissionsManager.shared.microphoneGranted ? Color.voxDone : Color.voxRecording)
                }
                HStack {
                    Text("Accessibility")
                    Spacer()
                    Text(PermissionsManager.shared.accessibilityGranted ? "Granted" : "Denied")
                        .foregroundStyle(PermissionsManager.shared.accessibilityGranted ? Color.voxDone : Color.voxRecording)
                }
                Button("Open Privacy Settings") {
                    PermissionsManager.shared.openMicrophoneSettings()
                }
                #endif
            }

            Section("About") {
                HStack {
                    Text("Version")
                    Spacer()
                    Text("1.0 (Phase 1)")
                        .foregroundStyle(.secondary)
                }
                HStack {
                    Text("Telemetry")
                    Spacer()
                    Text("None — fully offline")
                        .foregroundStyle(.secondary)
                }
            }
        }
        .navigationTitle("Privacy")
        .formStyle(.grouped)
        .onAppear { PermissionsManager.shared.refreshStatus() }
    }
}
