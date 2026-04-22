import Foundation

// MARK: - AppContext

/// Contextual information about the active application at the time of dictation.
/// Used to inject relevant context into the cleanup prompt.
public struct AppContext: Sendable {
    public let bundleID: String
    public let appFamily: AppFamily
    public let appName: String

    public init(bundleID: String, appFamily: AppFamily, appName: String) {
        self.bundleID = bundleID
        self.appFamily = appFamily
        self.appName = appName
    }
}

// MARK: - AppFamily

public enum AppFamily: String, Sendable {
    case mail       // Apple Mail, Outlook, Spark
    case messages   // Messages, Slack, Teams, Discord, WhatsApp
    case code       // Xcode, VS Code, Cursor, Terminal
    case notes      // Notes, Notion, Obsidian, Bear
    case browser    // Safari, Chrome, Firefox, Arc
    case docs       // Pages, Word, Google Docs
    case generic    // Everything else

    /// A hint string injected into the cleanup prompt.
    var promptHint: String? {
        switch self {
        case .mail:     return "The user is composing an email. Use a professional, polished tone."
        case .messages: return "The user is writing a chat message. Keep it conversational and natural."
        case .code:     return "The user is in a code editor. Preserve technical terms, variable names, and identifiers exactly."
        case .notes:    return "The user is writing personal notes. Keep it clean but informal."
        case .docs:     return "The user is writing a document. Use clear, structured prose."
        case .browser:  return nil
        case .generic:  return nil
        }
    }
}

// MARK: - PromptBuilder

/// Builds system + user prompts for the cleanup and command pipelines.
public struct PromptBuilder: Sendable {

    private static let cleanupSystemPrompt: String = {
        let url = Bundle.main.url(forResource: "cleanup.system", withExtension: "txt",
                                  subdirectory: "Prompts")
            ?? Bundle.main.url(forResource: "cleanup.system", withExtension: "txt")
        return (url.flatMap { try? String(contentsOf: $0, encoding: .utf8) }) ?? Self.fallbackCleanupPrompt
    }()

    private static let commandSystemPrompt: String = {
        let url = Bundle.main.url(forResource: "command.system", withExtension: "txt",
                                  subdirectory: "Prompts")
            ?? Bundle.main.url(forResource: "command.system", withExtension: "txt")
        return (url.flatMap { try? String(contentsOf: $0, encoding: .utf8) }) ?? Self.fallbackCommandPrompt
    }()

    // MARK: - Public API

    /// Builds a cleanup prompt for the given raw transcript and optional app context.
    public static func cleanupPrompt(rawTranscript: String, context: AppContext?) -> Prompt {
        var system = cleanupSystemPrompt
        if let hint = context?.appFamily.promptHint {
            system += "\n\n\(hint)"
        }
        return Prompt(system: system, user: rawTranscript)
    }

    /// Builds a command prompt for editing selected text with a spoken instruction.
    public static func commandPrompt(selection: String, instruction: String, context: AppContext?) -> Prompt {
        var system = commandSystemPrompt
        if let hint = context?.appFamily.promptHint {
            system += "\n\n\(hint)"
        }
        let user = """
        Text to edit:
        \(selection)

        Instruction: \(instruction)
        """
        return Prompt(system: system, user: user)
    }

    // MARK: - Fallbacks (if prompt files not found in bundle)

    private static let fallbackCleanupPrompt = """
    You are a transcription cleanup assistant. Remove filler words, fix punctuation and capitalization, and preserve the speaker's voice. Return only the cleaned text.
    """

    private static let fallbackCommandPrompt = """
    You are a text editing assistant. Apply the user's instruction to the provided text. Return only the edited result.
    """
}
