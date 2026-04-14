# CLAUDE.md

Guidance for Claude Code (and compatible agents) when operating in this repository.

---

## Project Snapshot

- **Name:** Vox (codename, Meridian LLC)
- **What:** Privacy-first, offline voice dictation for iOS 18+ / macOS 15+
- **Pipeline:** Whisper (STT) → Gemma 4 (LLM cleanup) → paste at cursor
- **Stack:** SwiftUI (multiplatform), SwiftData, whisper.cpp + llama.cpp via SPM, Metal / Core ML
- **Business model:** Free tier + $29.99 one-time Pro unlock
- **Competitive target:** Wispr Flow, but local, cheaper, and private

If you haven't yet, read these three files first — they are the source of truth:

1. [`vox-prd-v1.md`](vox-prd-v1.md) — The product requirements document
2. [`vox-open-questions-resolved.md`](vox-open-questions-resolved.md) — Architecture decisions
3. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — System design

---

## Repository Layout

```
.
├── README.md, CLAUDE.md, AGENTS.md       Top-level docs
├── vox-prd-v1.md                          PRD (canonical)
├── vox-open-questions-resolved.md         Architecture rationale
├── docs/
│   ├── ARCHITECTURE.md
│   ├── ROADMAP.md
│   ├── IMPLEMENTATION_PLAN.md
│   └── diagrams/
│       └── pipeline.md
└── (future) Vox.xcodeproj, Vox/, VoxTests/, VoxUITests/, Packages/
```

No Swift source yet. The Xcode project is scheduled for **Phase 1, Sprint 0** in the implementation plan.

---

## How to Work in This Repo

### Before writing code

1. **Read the PRD section that applies** before modifying or adding a feature. Feature-level acceptance criteria live in `vox-prd-v1.md` §7.
2. **Check the roadmap phase.** Features are phased P0 / P1 / P2. Don't build P1 features while P0 is incomplete unless explicitly asked.
3. **Confirm the model tier.** iOS = Gemma 4 E2B + Whisper small.en. Mac = Gemma 4 E4B + Whisper medium.en. Device detection drives tier selection.

### When touching the inference pipeline

- **Never** add a network call to the dictation path without explicit approval. The product is "no audio leaves the device." Cloud BYOK is **opt-in only** and disabled by default (Phase 2).
- Keep audio buffers ephemeral — clear after each dictation.
- Model files are **not** bundled in the app binary. They are downloaded on first launch into the App Group shared container.
- Prefer `whisper.cpp` + `llama.cpp` with Metal for Phase 1. Core ML conversion is a Phase 2 optimization.

### When touching UI

- SwiftUI first. AppKit / UIKit only when SwiftUI cannot do the job (e.g. global hotkey capture on macOS).
- Multiplatform targets — `#if os(iOS)` / `#if os(macOS)` blocks are acceptable but prefer platform-specific files (`FooView+iOS.swift`, `FooView+macOS.swift`) when divergence is substantial.
- Use SF Symbols; no custom iconography until branding is locked.

### When touching storage

- SwiftData for v1. **Do not** enable CloudKit until Phase 3.
- Keychain for BYOK API keys only.
- App Group shared container for anything the (future) keyboard extension must read.

---

## Coding Conventions

- **Swift version:** Swift 6 (strict concurrency enabled).
- **Indentation:** 4 spaces. No tabs.
- **Line length:** Soft 120.
- **Naming:** Apple API Design Guidelines. `VoxFoo` prefix is not used — rely on module namespacing.
- **Error handling:** Throwing functions + typed errors where practical. Never fatal-error on user-reachable paths.
- **Logging:** `os.Logger` with subsystem `llc.meridian.vox` and meaningful categories (`pipeline`, `audio`, `inference`, `ui`). **Never** log audio, transcripts, or cleaned text at any level above `.debug` — PII.
- **Concurrency:** Structured concurrency (`async`/`await`, Task groups). Avoid `DispatchQueue` unless bridging to callback APIs. Isolate inference on a dedicated serial actor.
- **Testing:** XCTest + Swift Testing where it fits. Aim for ≥ 70% coverage on non-UI logic. Golden-file tests for cleanup prompts.

---

## Running & Testing (Once the Project Exists)

Expected commands (documented now so agents don't invent their own):

```bash
# Build
xcodebuild -scheme Vox -destination 'platform=macOS' build
xcodebuild -scheme Vox -destination 'platform=iOS Simulator,name=iPhone 16' build

# Test
xcodebuild -scheme Vox -destination 'platform=macOS' test

# Lint / format
swiftformat .
swiftlint
```

When Claude Code opens this repo, the `SessionStart` hook (see `docs/IMPLEMENTATION_PLAN.md` Sprint 0) should verify `xcodebuild`, `swiftformat`, and `swiftlint` are available. See the **Skill Recommendations** section below.

---

## PR & Commit Conventions

- **Branches:** `claude/<short-slug>` for Claude-driven work. Feature branches: `feature/<slug>`. Fixes: `fix/<slug>`.
- **Commits:** Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`). Describe *why* in the body, not just *what*.
- **PRs:** Keep under ~400 changed lines where possible. Include:
  - What / Why summary
  - Screenshots or short screen recordings for UI changes
  - Test plan (manual + automated)
  - PRD section reference (e.g. "implements §7 Command Mode")
- **Do not** open a PR unless the user explicitly asks for one.

---

## Guardrails for Claude

1. **Don't invent features.** If it's not in the PRD or roadmap, ask before building.
2. **Don't guess model identifiers, file paths, or SDK versions.** Confirm or read first.
3. **Don't add analytics, telemetry, or crash reporting** — explicitly out of scope for v1.
4. **Don't bundle model weights** in the app binary.
5. **Don't alter privacy posture** (e.g. adding an opt-out analytics flag, moving inference to the cloud) without user approval.
6. **Don't change the monetization model** — the free + one-time Pro tier is a positioning decision.
7. **Don't skip iOS memory budgets.** iOS Jetsam is unforgiving; stay under the tier's RAM target.
8. **Think before running destructive git ops.** See the GitHub section below.

---

## GitHub

This repo is `ryan-richmond/speechtotext`. GitHub operations happen via the `mcp__github__*` MCP tools (not `gh` CLI). The working branch for Claude-driven documentation is `claude/project-documentation-setup-zosoA`.

- Never force-push `main`.
- Never bypass hooks (`--no-verify`).
- Never open a PR without explicit request.

---

## Skill Recommendations

The following agent skills would accelerate Vox development. See [AGENTS.md](AGENTS.md) for the full list:

- **`session-start-hook`** — Already available. Use it to ensure `xcodebuild`, `swiftformat`, `swiftlint`, and model files exist at session start.
- **`update-config`** — For wiring up hooks (e.g. run `swiftformat` on save, run tests on commit).
- **`simplify`** — Post-implementation review for Swift code (especially inference pipeline).
- **`claude-api`** — Relevant for BYOK cloud fallback (Phase 2), not for core on-device work.

Recommended **new skills** to author for this project:

1. **`vox-model-bench`** — Benchmark a Whisper / Gemma model on the current device and output a table of WER, latency, RAM, and battery impact. Runs via `xcodebuild test -only-testing Vox.Benchmarks`.
2. **`vox-prompt-eval`** — Run the cleanup system prompt against a golden set of raw transcripts and score against reference outputs. Critical for Phase 1 prompt engineering and Phase 2 tone-matching.
3. **`vox-privacy-audit`** — Grep the codebase for network calls, analytics SDKs, and PII logging. Fails CI if anything slips in.

Ask the user before scaffolding any of these — they each deserve their own scoping pass.

---

## Where to Ask Questions

- Implementation uncertainty → `docs/IMPLEMENTATION_PLAN.md` first, then ask.
- Scope uncertainty → `docs/ROADMAP.md` first, then ask.
- Architecture uncertainty → `docs/ARCHITECTURE.md` + `vox-open-questions-resolved.md`, then ask.
- Anything involving shipped user impact (paste targets, hotkey changes, privacy posture, monetization) → always ask.
