import SwiftUI
import SwiftData

// MARK: - HistoryView

public struct HistoryView: View {
    @Query(sort: \Transcription.createdAt, order: .reverse) private var transcriptions: [Transcription]
    @Environment(\.modelContext) private var modelContext
    @State private var searchText = ""
    @State private var selectedID: UUID?

    public var body: some View {
        NavigationStack {
            List(filteredTranscriptions, id: \.id, selection: $selectedID) { item in
                TranscriptionRow(item: item)
                    .swipeActions(edge: .trailing, allowsFullSwipe: true) {
                        Button(role: .destructive) {
                            modelContext.delete(item)
                        } label: {
                            Label("Delete", systemImage: "trash")
                        }
                    }
                    .contextMenu {
                        Button {
                            #if os(macOS)
                            NSPasteboard.general.clearContents()
                            NSPasteboard.general.setString(item.cleanedText, forType: .string)
                            #else
                            UIPasteboard.general.string = item.cleanedText
                            #endif
                        } label: {
                            Label("Copy", systemImage: "doc.on.doc")
                        }
                    }
            }
            .navigationTitle("History")
            .searchable(text: $searchText, prompt: "Search transcriptions")
            .overlay {
                if transcriptions.isEmpty {
                    ContentUnavailableView(
                        "No Dictations Yet",
                        systemImage: "waveform",
                        description: Text("Your transcription history will appear here.")
                    )
                }
            }
        }
    }

    private var filteredTranscriptions: [Transcription] {
        guard !searchText.isEmpty else { return transcriptions }
        return transcriptions.filter {
            $0.cleanedText.localizedCaseInsensitiveContains(searchText) ||
            ($0.appName ?? "").localizedCaseInsensitiveContains(searchText)
        }
    }
}

// MARK: - TranscriptionRow

private struct TranscriptionRow: View {
    let item: Transcription

    var body: some View {
        VStack(alignment: .leading, spacing: Spacing.xs) {
            Text(item.cleanedText)
                .font(.voxBody)
                .lineLimit(2)

            HStack {
                if let app = item.appName {
                    Label(app, systemImage: "app.badge")
                        .font(.voxCaption)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                Text(item.createdAt.formatted(.relative(presentation: .named)))
                    .font(.voxCaption)
                    .foregroundStyle(.tertiary)
                Text("·")
                    .foregroundStyle(.tertiary)
                Text("\(item.latencyMs)ms")
                    .font(.voxCaption.monospacedDigit())
                    .foregroundStyle(.tertiary)
            }
        }
        .padding(.vertical, Spacing.xs)
    }
}
