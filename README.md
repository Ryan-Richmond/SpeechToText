# Vox

> Privacy-first, offline-capable voice dictation for iOS and macOS.

**Status:** Sprint 0 in progress (initial code scaffold added)
**Platform:** iOS 18+ / macOS 15+ (Sequoia) — SwiftUI multiplatform
**License:** Proprietary (Meridian LLC) — built on Apache 2.0 / MIT open-source models

---

## What is Vox?

Vox turns natural speech into polished, context-aware text anywhere on iOS and macOS — without a cloud subscription and without sending a byte of audio off your device.

It is a ground-up competitor to [Wispr Flow](https://wisprflow.ai/), with two core differences:

1. **100% on-device.** All speech recognition and language model inference runs locally via Whisper + Gemma 4, using Apple Silicon (Metal + Neural Engine).
2. **No subscription.** One-time Pro unlock instead of recurring fees.

## Key Features

- **Global Dictation** — Press a hotkey (Mac) or tap the Action Button / keyboard (iOS). Audio is captured, transcribed by Whisper, cleaned up by Gemma, and pasted at the cursor.
- **AI Text Cleanup** — Removes filler words, adds punctuation, fixes grammar, resolves self-corrections ("4pm, actually 3pm" → "3pm").
- **Context-Aware Formatting** — Detects the active app (Mail, Messages, Slack, Xcode, etc.) and adjusts tone and formatting accordingly.
- **Command Mode** — Select text + hold hotkey + speak a command ("make this friendlier," "turn this into bullets") to rewrite in place.
- **Personal Dictionary & Snippets** — Teach Vox your names, acronyms, and expansions.
- **Tone Matching** — Learns your writing style over time for outputs that sound like *you*.

## Pipeline

```
Microphone → AVAudioEngine
  → Stage 1: Whisper (whisper.cpp + Metal)  → raw transcript
  → Stage 2: Gemma 4 (llama.cpp + Metal)    → polished text
  → Clipboard / Cmd+V simulation            → cursor
```

Two-stage by default; an optional single-stage "Lite Mode" uses Gemma 4 end-to-end (audio → text) for constrained devices.

## Model Tiers

| Platform | STT | Cleanup LLM | RAM Budget |
|----------|-----|-------------|------------|
| iPhone 15 Pro / 16 (8GB) | Whisper small.en (~250MB) | Gemma 4 **E2B** Q4_K_M (~1.5GB) | ~1.9GB |
| M1 Mac (16GB) | Whisper medium.en (~750MB) | Gemma 4 **E4B** Q4_K_M (~3.5GB) | ~4.5GB |
| M3 Mac (36GB+) | Whisper large-v3 (~1.5GB) | Gemma 4 E4B Q8_0 (~7GB) | ~9GB |

An adaptive selector picks the right tier at first launch based on `ProcessInfo.processInfo.physicalMemory`.

## Requirements

- **iPhone:** iPhone 15 Pro or later (8GB RAM, A17 Pro+ Neural Engine), iOS 18+
- **Mac:** Apple Silicon (M1 or later), 16GB RAM, macOS 15 Sequoia+
- **Disk (first launch download):** ~1.25GB (iOS) or ~3.75GB (Mac)

## Repository Structure

```
.
├── README.md                        You are here
├── CLAUDE.md                        Guidance for Claude Code sessions
├── AGENTS.md                        Conventions for AI coding agents
├── vox-prd-v1.md                    Original product requirements document
├── vox-open-questions-resolved.md   Architecture decisions / open questions
└── docs/
    ├── ARCHITECTURE.md              System architecture & data flow
    ├── ROADMAP.md                   Product roadmap by phase
    ├── IMPLEMENTATION_PLAN.md       Engineering plan, sprint-by-sprint
    └── diagrams/
        └── pipeline.md              ASCII / Mermaid diagrams
```

A Sprint 0 Xcode bootstrap spec is now in-repo (`project.yml`) with generation via `xcodegen` (`scripts/bootstrap_xcodeproj.sh`).

## Getting Started (Developers)

> Generate `Vox.xcodeproj` with `scripts/bootstrap_xcodeproj.sh` (requires `xcodegen`).

Once the project is bootstrapped:

```bash
# Clone
git clone https://github.com/ryan-richmond/speechtotext.git
cd speechtotext

# Open in Xcode
open Vox.xcodeproj

# Or build from CLI
xcodebuild -scheme Vox -configuration Debug build
```

## Documentation Map

- **[PRD v1](vox-prd-v1.md)** — The source product requirements.
- **[Open Questions Resolved](vox-open-questions-resolved.md)** — Architecture decisions and rationale.
- **[Architecture](docs/ARCHITECTURE.md)** — System design and diagrams.
- **[Roadmap](docs/ROADMAP.md)** — What we're building, in what order, and why.
- **[Implementation Plan](docs/IMPLEMENTATION_PLAN.md)** — Sprint-by-sprint engineering plan.
- **[CLAUDE.md](CLAUDE.md)** — Instructions for Claude Code sessions.
- **[AGENTS.md](AGENTS.md)** — Conventions for AI agents working in this repo.

## Positioning

| Product | Price | Offline | Open Models | Platforms |
|---------|-------|---------|-------------|-----------|
| Wispr Flow | $15/mo | No | No | iOS, Mac, Win |
| Superwhisper | $85/yr or $249 | Yes | No | Mac |
| OpenWhispr | Free | Yes | Yes | Mac (Electron) |
| VoiceInk | $39 one-time | Yes | Yes | Mac |
| Apple Dictation | Free | Yes | No | iOS, Mac |
| **Vox** | Free / $29.99 Pro | **Yes** | **Yes** | **iOS + Mac** |

## License

Proprietary to Meridian LLC. Source code is not open-source, though Vox is built on top of Apache 2.0 (Gemma) and MIT (Whisper) models.
