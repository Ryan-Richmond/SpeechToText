# AGENTS.md

Conventions for AI coding agents (Claude Code, Cursor, Codex, Aider, etc.) working in the Vox repository. This file is the agent-agnostic sibling of [`CLAUDE.md`](CLAUDE.md) — it aims to be a good `AGENTS.md` per the emerging community spec.

---

## 1. Orientation

**What this repo is:** Planning and (soon) source for **Vox**, an on-device voice dictation app for iOS 18+ / macOS 15+. See [`README.md`](README.md) for the user-facing summary and [`vox-prd-v1.md`](vox-prd-v1.md) for the canonical product spec.

**Current state:** Documentation-only. Xcode project is scheduled for Phase 1, Sprint 0 (see [`docs/IMPLEMENTATION_PLAN.md`](docs/IMPLEMENTATION_PLAN.md)).

**North star:**
> A user presses a hotkey, speaks, and polished text appears at their cursor — within 2 seconds, with zero network traffic, on hardware they already own.

Every design decision is in service of that sentence.

---

## 2. Golden Rules

1. **Offline by default.** No feature may introduce a required network call to the dictation hot path. Cloud fallback is Phase 2, opt-in, BYOK only.
2. **Respect memory budgets.** iOS ≤ 1.9GB resident during dictation, Mac ≤ 5GB. Exceeding triggers Jetsam on iOS — catastrophic UX.
3. **Respect phases.** Don't build P1 while P0 is unfinished. Don't build P2 ever without user say-so.
4. **Don't guess the spec.** If `vox-prd-v1.md` doesn't answer your question, ask the user — don't invent.
5. **Privacy is the product.** No analytics, telemetry, crash reporting, or PII in logs in v1.
6. **Ask before risky actions.** Destructive git ops, dependency additions, entitlement changes, monetization changes, and anything that alters the privacy story require approval.

---

## 3. Repository Layout

```
.
├── README.md                         Public-facing overview
├── CLAUDE.md                         Claude Code specifics
├── AGENTS.md                         ← You are here (all agents)
├── vox-prd-v1.md                     Canonical product requirements
├── vox-open-questions-resolved.md    Architecture decisions + rationale
└── docs/
    ├── ARCHITECTURE.md               System design + diagrams
    ├── ROADMAP.md                    Phased roadmap
    ├── IMPLEMENTATION_PLAN.md        Sprint-by-sprint engineering plan
    └── diagrams/
        └── pipeline.md               Mermaid / ASCII diagrams
```

Future (post Sprint 0):

```
Vox.xcodeproj/
Vox/
  App/                 App entry, scene, lifecycle
  Features/            Dictation, Command Mode, History, Settings, Onboarding
  Pipeline/            Audio, STT, LLM, Paste — the inference pipeline
  Models/              SwiftData entities
  Services/            ModelDownload, Permissions, HotkeyManager
  UI/                  Shared views, design system
  Resources/           Assets, localized strings, prompts
VoxTests/
VoxUITests/
VoxKeyboard/           iOS keyboard extension (Phase 2)
Packages/              whisper.cpp, llama.cpp wrapper packages
scripts/               Build, model download, benchmarking
```

---

## 4. Tech Stack (Authoritative)

| Layer | Choice | Notes |
|---|---|---|
| UI | SwiftUI | Multiplatform, iOS 18+ / macOS 15+ |
| Persistence | SwiftData | Local only for v1 |
| Audio capture | AVFoundation / AVAudioEngine | |
| STT | whisper.cpp | Metal acceleration; Core ML Phase 2 |
| LLM | llama.cpp | GGUF Q4_K_M quantization, Metal |
| Concurrency | Swift structured concurrency | `async`/`await`, actors |
| Hotkeys (macOS) | Carbon / `NSEvent.addGlobalMonitor` | Requires Accessibility |
| Paste (macOS) | `NSPasteboard` + simulated Cmd+V | |
| iOS input (MVP) | Action Button / Shortcuts | Keyboard extension in v1.1 |
| iOS input (v1.1) | Keyboard extension "trampoline" | Jumps to main app for inference |
| Secret storage | Keychain | BYOK API keys only |
| Logging | `os.Logger` | Subsystem: `llc.meridian.vox` |

**Dependencies are added reluctantly.** Prefer Apple SDKs and the two `.cpp` libraries above. Every new SPM package requires a written justification in the PR description.

---

## 5. Coding Conventions

- **Swift 6** with strict concurrency.
- **4-space indentation**, no tabs. Soft 120-char lines.
- **Apple API Design Guidelines** for naming. No `VoxFoo` prefix.
- **One type per file** unless types are trivially small (`enum` + `struct` pairs).
- **Explicit `public` / `internal` / `private`.** Err on the side of `private`.
- **`@MainActor`** for UI. Dedicated actor (`PipelineActor`) for STT + LLM orchestration.
- **Formatter:** `swiftformat` (config in `.swiftformat`).
- **Linter:** `swiftlint` (config in `.swiftlint.yml`).
- **Tests:** XCTest (and Swift Testing where it fits). `VoxTests/Fixtures/` holds audio + transcript goldens.

## 6. Build & Test

Expected commands (canonicalized here so agents don't invent their own):

```bash
# Build
xcodebuild -scheme Vox -destination 'platform=macOS' build
xcodebuild -scheme Vox -destination 'platform=iOS Simulator,name=iPhone 16' build

# Unit + integration tests
xcodebuild -scheme Vox -destination 'platform=macOS' test

# Format + lint
swiftformat .
swiftlint

# Benchmarks
xcodebuild test -scheme Vox -only-testing:VoxTests/Benchmarks
```

A `Makefile` (or `just` file) wrapping these lands in Sprint 0.

## 7. Commit & PR Conventions

- **Conventional Commits:** `feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`, `perf:`.
- **Body** explains *why*, not just *what*. Reference PRD section numbers.
- **Branches:**
  - `claude/<slug>` — Claude-driven
  - `feature/<slug>` — human or other agents
  - `fix/<slug>` — bugfixes
  - `docs/<slug>` — doc-only
- **PRs:** only when the user explicitly asks. Keep under ~400 LOC. Include test plan, screenshots for UI changes, and a "Risk" line (low / medium / high, with 1-sentence justification).

## 8. Definition of Done

A change is **done** when all of these are true:

- [ ] Code compiles for both iOS and macOS schemes
- [ ] `swiftformat` and `swiftlint` pass
- [ ] Unit tests pass; new logic has new tests
- [ ] No new required network calls in the dictation path
- [ ] No PII in logs
- [ ] RAM budget respected (run on-device or via Instruments if feasible)
- [ ] Relevant doc updated (PRD reference, README if user-visible, ARCHITECTURE if structural)
- [ ] PR (if opened) references the PRD section implemented

## 9. Guardrails

- **Don't add analytics or telemetry.** Not even opt-in in v1.
- **Don't bundle model weights.** Models ship via first-launch download.
- **Don't store audio on disk** past the life of a single dictation request.
- **Don't add CloudKit** until Phase 3.
- **Don't change entitlements** (Accessibility, Mic, Full Access, App Groups) without flagging it in the PR description.
- **Don't introduce a subscription SKU.** Free + one-time Pro only.
- **Don't touch `main` branch protection or CI config** without approval.

## 10. Recommended Agent Skills

These skills (Claude Code skill format, but the ideas transfer) would materially accelerate Vox work. The first three are off-the-shelf; the last three should be authored.

### Available (enable when useful)

| Skill | Why it's useful here |
|-------|----------------------|
| `session-start-hook` | Verify toolchain (`xcodebuild`, `swiftformat`, `swiftlint`) and model files on session start. |
| `update-config` | Configure Claude Code hooks: format-on-save, run tests pre-commit, run privacy-audit pre-push. |
| `simplify` | Post-implementation review on Swift changes — especially inference orchestration. |

### Recommended for this project (to author)

| Skill | What it does | Rough trigger |
|-------|--------------|---------------|
| `vox-model-bench` | Benchmarks a Whisper or Gemma variant on the host device (WER, latency, RAM, thermals). Writes results to `docs/benchmarks/`. | "benchmark small.en", "how fast is E4B on this Mac" |
| `vox-prompt-eval` | Runs the cleanup system prompt against `VoxTests/Fixtures/cleanup_goldens/` and produces pass/fail + diff. | "evaluate the cleanup prompt", after any prompt change |
| `vox-privacy-audit` | Greps for `URLSession`, known analytics SDKs, NSLog/print of PII, and checks Info.plist for analytics identifiers. Fails loud. | Pre-push, manual invocation |
| `vox-release-checklist` | Runs the App Store pre-submission list from `docs/ROADMAP.md` §Release. | "run release checklist" |
| `vox-entitlement-diff` | Diffs entitlements and Info.plist against last tagged release. | Pre-PR, pre-release |

Ask the user before scaffolding any of these — each deserves its own design pass.

## 11. Useful Context Files

When starting a new task, the most useful files to load first are:

1. `vox-prd-v1.md` §7 (Core Features) — for feature work
2. `vox-open-questions-resolved.md` §1, §2 — for iOS / model tier decisions
3. `docs/ARCHITECTURE.md` — for anything structural
4. `docs/IMPLEMENTATION_PLAN.md` — for the current sprint and phase
5. `docs/ROADMAP.md` — for phase classification (P0/P1/P2)
