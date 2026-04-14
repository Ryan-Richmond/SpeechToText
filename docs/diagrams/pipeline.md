# Vox — Diagrams

Canonical source for Mermaid diagrams used throughout Vox docs. Rendered on GitHub and most Markdown tools.

---

## 1. End-to-End Dictation Pipeline (Two-Stage, default)

```mermaid
flowchart LR
    subgraph User
        M[Microphone]
    end

    subgraph Capture
        AE[AVAudioEngine<br/>16kHz mono PCM]
        VAD[VAD / silence trim]
    end

    subgraph STT
        W[Whisper<br/>whisper.cpp + Metal]
    end

    subgraph LLM
        PB[PromptBuilder<br/>+ app context<br/>+ dictionary]
        G[Gemma 4<br/>llama.cpp + Metal]
    end

    subgraph Output
        PS[PasteService]
        TF[Active text field]
    end

    M --> AE --> VAD --> W -- raw transcript --> PB --> G -- cleaned text --> PS --> TF
```

---

## 2. Runtime Topology — macOS

```mermaid
flowchart TB
    subgraph Vox.app
        UI[SwiftUI @MainActor<br/>Menu bar · Overlay · Prefs]
        PA[PipelineActor<br/>serial, non-main]
        SV[Services<br/>Hotkey · Permissions · Download<br/>Paste · AppContext]
        SD[(SwiftData<br/>Transcriptions, Dictionary, …)]
        KC[(Keychain<br/>BYOK, Phase 2)]
    end

    OS[macOS system<br/>Accessibility, Mic, NSWorkspace]
    MF[(Models on disk<br/>~/Library/Application Support/llc.meridian.vox/models)]

    UI <--> PA
    UI <--> SV
    PA --> MF
    SV <--> OS
    PA <--> SD
    SV <--> KC
```

---

## 3. Runtime Topology — iOS (MVP, no keyboard extension)

```mermaid
flowchart TB
    subgraph Vox.app
        UI[SwiftUI UI]
        PA[PipelineActor]
        SV[Services]
        SD[(SwiftData)]
    end

    AB[Action Button / Shortcut]
    CL[(UIPasteboard)]
    AG[(App Group container<br/>models + shared data)]

    AB --> UI --> PA --> AG
    PA --> CL
    UI <--> SV
    PA <--> SD
```

---

## 4. Runtime Topology — iOS (v1.1, keyboard extension trampoline)

```mermaid
flowchart LR
    subgraph Keyboard[VoxKeyboard extension]
        KB[Mic button<br/>thin shell]
    end
    subgraph Main[Vox.app]
        PA2[Pipeline<br/>STT + LLM]
    end
    subgraph Shared[App Group]
        AG2[(Cleaned text<br/>+ status flag)]
    end
    TXT[Active text field]

    KB -- deep link vox://dictate --> PA2
    PA2 -- write result --> AG2
    AG2 -- read --> KB
    KB -- insertText --> TXT
```

---

## 5. Model Tier Selection

```mermaid
flowchart TD
    S[App launch] --> C{ProcessInfo.physicalMemory}
    C -- ≥ 32 GB Mac --> P[Power:<br/>Whisper large-v3 +<br/>Gemma 4 E4B Q8_0]
    C -- 16 GB Mac --> D[Default Mac:<br/>Whisper medium.en +<br/>Gemma 4 E4B Q4_K_M]
    C -- 8 GB iPhone --> I[Default iOS:<br/>Whisper small.en +<br/>Gemma 4 E2B Q4_K_M]
    C -- < 8 GB --> X[Unsupported<br/>show upgrade dialog]

    P & D & I --> U[User can override<br/>Settings → Models]
```

---

## 6. Command Mode Flow

```mermaid
sequenceDiagram
    participant U as User
    participant OS as macOS
    participant V as Vox
    participant G as Gemma 4

    U->>OS: Select text in any app
    U->>V: Hold Command Mode hotkey
    U->>V: Speak command ("make this friendlier")
    V->>OS: Read selection via Accessibility API
    V->>V: Whisper transcribes command
    V->>G: system=command prompt, user=(selection, command)
    G-->>V: rewritten text
    V->>OS: Replace selection via paste
    U->>U: Sees rewritten text in place
```

---

## 7. First-Launch Download

```mermaid
sequenceDiagram
    participant U as User
    participant V as Vox
    participant N as CDN
    participant D as Disk

    U->>V: Launches app (first time)
    V->>U: "Download AI models to use Vox (≈3.75 GB)"
    U->>V: Confirm
    V->>N: Request Whisper model (ranged)
    N-->>V: Stream + Content-Length
    V->>D: Write to models/ with .partial suffix
    V->>V: SHA-256 verify
    V->>N: Request Gemma model (ranged)
    N-->>V: Stream
    V->>D: Write + verify
    V->>V: Register ModelConfig in SwiftData
    V->>U: "Ready. Press Fn twice to start dictating."
```
