# Vox — Implementation Plan

**Audience:** The engineer (or agent) executing the build.
**Companion:** [`ROADMAP.md`](ROADMAP.md) for the *product* sequencing; this doc is the *engineering* sequencing.
**Last updated:** 2026-04-14

Sprints are 2 weeks. Capacity assumption: **one full-time engineer (Rodge)** with AI-agent assistance. Numbers are targets, not guarantees.

---

## Phase 0 — Foundation ✅ (current)

**Outcome:** Documentation complete, decisions locked, repo ready for Sprint 0.

- [x] PRD v1 (`vox-prd-v1.md`)
- [x] Open-question resolutions (`vox-open-questions-resolved.md`)
- [x] README, CLAUDE.md, AGENTS.md
- [x] Architecture doc
- [x] Roadmap
- [x] Implementation plan (this doc)
- [ ] Eval dataset: 50+ audio clips with reference transcripts and "ideal cleaned" outputs. Stored in `VoxTests/Fixtures/eval/` (LFS).
- [ ] Naming / trademark pre-check

---

## Phase 1 — MVP

### Sprint 0 — Project Bootstrap (1 week)

**Goal:** An empty but correctly configured SwiftUI multiplatform project builds cleanly on iOS 18 + macOS 15.

**Tasks:**

1. Create `Vox.xcodeproj` (multiplatform target).
2. Enable Swift 6 with strict concurrency.
3. Wire `.swiftformat`, `.swiftlint.yml`, `.editorconfig`.
4. Add SPM packages (initially empty wrappers): `WhisperKit` / `WhisperCpp`, `LlamaCpp`.
5. App Group configured (`group.llc.meridian.vox`).
6. Entitlements: Mic, Accessibility (Mac), App Groups, Hardened Runtime.
7. Logging scaffolding (`os.Logger` + category enums).
8. `SessionStart` hook verifying `xcodebuild`, `swiftformat`, `swiftlint`, disk space.
9. CI: GitHub Actions job — `build` + `test` for both schemes, `swiftlint`, privacy grep.
10. `Makefile` / `justfile` wrapping common commands.

**Exit criteria:**
- `make build` produces an iOS + macOS `.app` with a blank launch screen.
- `make test` runs an empty test target and passes.
- `make lint` passes.

---

### Sprint 1 — Audio + STT (2 weeks)

**Goal:** Record audio with hotkey (Mac) and tap (iOS), transcribe with Whisper, display raw text in a debug view.

**Tasks:**

1. `AudioCapture` actor using AVAudioEngine; produces 16kHz mono PCM.
2. VAD / silence trimming (simple RMS threshold; upgrade later).
3. `WhisperCppEngine` — Swift wrapper for `whisper.cpp`, Metal enabled.
4. `ModelDownloadService` — resumable download, SHA-256 verify, disk-space preflight.
5. First-launch flow: download small.en (iOS) / medium.en (Mac) to App Group container.
6. Debug view: tap/hotkey → capture → transcribe → show text.
7. macOS global hotkey via `NSEvent.addGlobalMonitorForEvents` or Carbon `RegisterEventHotKey`.
8. iOS: in-app "Dictate" button (no system integration yet).
9. `DeviceCapabilityService` picks tier from `ProcessInfo.physicalMemory`.
10. Unit tests for `AudioCapture` (fake engine) and `ModelDownloadService`.

**Exit criteria:**
- Record 10 seconds of clean speech; Whisper returns text within latency target.
- Permission denials handled with user-visible prompts + Settings deep-link.
- Models ≤ 1.25 GB (iOS) / 3.75 GB (Mac) downloaded and verified.

**Risks:** `whisper.cpp` + Metal quirks. Mitigation: pin to a known-good release; vendor if needed.

---

### Sprint 2 — LLM Cleanup + Prompt (2 weeks)

**Goal:** Whisper output flows through Gemma 4 cleanup; final text is coherent.

**Tasks:**

1. `LlamaCppEngine` — Swift wrapper, Metal, Q4_K_M GGUF loader.
2. Download Gemma 4 E4B (Mac) / E2B (iOS) on first launch. Pause/resume.
3. `PromptBuilder.cleanupPrompt(context:, dictionary:)`.
4. Write v1 cleanup system prompt (PRD §7 draft is the starting point).
5. `PipelineActor.dictate(audio:context:)` orchestrates STT → LLM → return string.
6. First version of `cleanup_goldens/` fixtures (20+ examples).
7. `vox-prompt-eval` harness (scripts/): run prompt against goldens, diff output.
8. Debug view shows raw + cleaned side-by-side for comparison.

**Exit criteria:**
- End-to-end latency ≤ 2.5s (Mac) / 3.5s (iOS) on 10s of speech.
- Prompt-eval pass rate ≥ 80% on cleanup goldens.
- Peak RAM within tier budget (measured via Instruments).

---

### Sprint 3 — Paste, Overlay, and Context (2 weeks)

**Goal:** Cleaned text lands in the active text field in any app.

**Tasks:**

1. `PasteService` (macOS): write to `NSPasteboard`, synthesize Cmd+V via `CGEvent`. Handle clipboard-restore option.
2. `PasteService` (iOS): write to `UIPasteboard` (MVP — user pastes manually; Shortcuts can automate).
3. `AppContextService` (macOS): `NSWorkspace` active app bundle ID → app family (Mail, Messages, Slack, IDE, ...).
4. Integrate context into prompt ("You are writing in Mail...").
5. Dictation overlay (SwiftUI floating pill): idle / recording / processing / inserted states.
6. Waveform animation during recording; shimmer during processing.
7. Menu-bar icon with state (idle / active / error).
8. First pass at Dictation History view.

**Exit criteria:**
- Dictation into Mail, Messages, Slack, Xcode works reliably on macOS.
- iOS paste copies cleaned text to pasteboard and notifies user via haptic + pill.

---

### Sprint 4 — Command Mode + Settings + Onboarding (2 weeks)

**Goal:** Command Mode works; users can configure Vox; first-run is polished.

**Tasks:**

1. Command Mode hotkey (macOS): hold-to-talk, requires a selection.
2. `PipelineActor.command(selection:audio:context:)`.
3. Command prompt template (separate from cleanup).
4. Settings: General (hotkey, launch at login), Models (download/manage), Privacy (clear history), About.
5. Onboarding flow: mic permission, Accessibility walkthrough (Mac), model download, "try it out".
6. History: search, filter by app, copy, delete, re-run.
7. Analytics-free crash *reproduction* path (logs stay local; user can share a zip).

**Exit criteria:**
- Command Mode works on 10/10 varied prompts ("make friendlier," "into bullets," etc.).
- Cold install → first dictation < 5 minutes including model download.

---

### Sprint 5 — Hardening + Benchmarks (2 weeks)

**Goal:** Quality bars hit. Benchmarks documented. Ready for personal daily-driver use.

**Tasks:**

1. Run `vox-model-bench` on iPhone 16, M1 Mac. Record WER, p50 / p95 latency, peak RAM, battery impact (iOS 30-min test).
2. Instruments pass: leaks, retain cycles, audio buffer growth.
3. Jetsam simulation (iOS: constrain via `memoryLimit` entitlement).
4. Accessibility pass: VoiceOver, Dynamic Type, high-contrast overlay.
5. Edge cases from PRD §7: no mic permission, silent audio, > 60s dictation (chunked).
6. App-specific prompt overrides (basic: Mail = formal, Messages = casual).
7. Dog-food for 1 week. Collect regressions. Fix P0s; file P1s.

**Exit criteria (MVP release to self):**
- WER ≤ 5%, latency p50 ≤ 3s (iOS) / 2s (Mac), peak RAM within budget.
- Zero crashes in 1 week of daily use.
- Privacy audit clean (zero network calls in dictation path).

---

## Phase 2 — v1.1 (Target: +4–6 weeks)

### Sprint 6 — Dictionary + Snippets (2 weeks)

- `DictionaryEntry` + `Snippet` SwiftData entities + UI.
- Dictionary hints injected into Whisper prompt (`--initial-prompt` equivalent).
- Auto-learn suggestion: after a user edits a dictation, diff vs original → offer to save terms.
- Snippet matching on cleaned text; exact trigger → expansion replacement.

### Sprint 7 — Tone Matching (2 weeks)

- `StyleProfile` entity: avg sentence length, formality score, top N vocab.
- Correction capture: when user edits a dictation, diff is appended to profile.
- Style context injection into cleanup prompt ("User tends to write short, direct sentences; avoids exclamation marks.").
- Reset profile button in Settings.

### Sprint 8 — Lite Mode + BYOK (2 weeks)

- Lite Mode: Gemma 4 end-to-end (audio encoder) path. Toggle in Settings.
- `CloudLLMEngine` — BYOK for Gemini 2.5 Flash-Lite and OpenAI. Keychain storage. Visible banner when active.
- Fallback logic: if local engine fails or user picks cloud default, route through cloud.
- Privacy reaffirmation: clearly show "this request is leaving your device" on every cloud call.

### Sprint 9 — iOS Keyboard Extension (2–3 weeks)

- Scaffold `VoxKeyboard` extension target.
- Thin shell UI: mic button, "dictate" label.
- Deep link into main app (`vox://dictate`).
- Main app: detect deep link → record → inference → write text to App Group → return.
- Keyboard: read from App Group → insert into text field.
- Test the "trampoline" on real hardware — this is the jankiest part, expect iteration.
- Onboarding addition: keyboard install + Full Access prompt.

### Sprint 10 — Polish + Beta Prep (2 weeks)

- Export: copy-all, share-sheet, Markdown export of transcription history.
- Error copy pass across the app.
- TestFlight build.
- Invite 10 beta users.
- Support loop: local-only log bundle export, GitHub issue template.

---

## Phase 3 — v2.0 (6–9 months out)

Sprint detail TBD — roadmap themes:

- Electron / Tauri port (new repo or monorepo decision required first).
- iCloud sync for dictionary / snippets / style profile (not history).
- Siri Shortcuts / Shortcuts intents for dictation, Command Mode.
- MCP server exposing `dictate` tool to Claude Code et al.
- Multi-language auto-detect.
- Whisper (quiet) mode.
- Meeting mode MVP (offline diarization).

---

## Phase 4 — Intelligence Layer (9–18 months out)

- LoRA fine-tuning path (MLX or llama.cpp training).
- Overnight / on-charger training scheduler.
- Agent mode (MCP-based integrations with Calendar, Mail, Reminders).
- Meeting summaries + action item extraction.

---

## Cross-Phase Engineering Work

Ongoing work that doesn't belong to a single sprint:

### Testing

- **Unit tests** every sprint; coverage target ≥ 70% on non-UI modules.
- **Golden tests** for the cleanup prompt — regression gates on every prompt change.
- **Snapshot tests** for key SwiftUI views.
- **Benchmark tests** in a separate scheme — run monthly and on every model change.

### CI

- Build + test on both schemes (PR gate).
- `swiftlint` (PR gate).
- `vox-privacy-audit` (PR gate).
- Benchmark job — on `main` only, nightly, publishes to `docs/benchmarks/`.
- Model-download smoke test (against a small pinned model) — weekly.

### Telemetry (for us, not our users)

- Developer-only `VoxDebug` build variant that logs verbose timings and writes to `~/Library/Logs/Vox/dev/`. Off in release.
- Never in release: analytics, crash SDKs, user identifiers.

### Release Process (per phase)

1. Branch cut: `release/vX.Y`.
2. Benchmark pass (all target devices).
3. Privacy + entitlement diff review.
4. Onboarding cold-install test.
5. TestFlight build → 48h soak.
6. App Store submission (Phase 2+).
7. Tag + release notes.

---

## Risks & Mitigations (engineering)

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| llama.cpp Metal regression | Medium | High | Pin commits. Vendor if necessary. |
| Whisper.cpp Metal regression | Low | High | Same. |
| iOS keyboard extension rejection | Low | Medium | Ship MVP without it; keyboard ext deferred to v1.1 where precedent (Wispr Flow) exists. |
| Gemma 4 release slips | Medium | Medium | Target Gemma 4 but keep Gemma 3n as the fallback default. |
| iOS Jetsam kills mid-dictation | Medium | High | Strict RAM budget; `increased-memory-limit` entitlement; user-surfaced "under memory pressure" path. |
| Hotkey conflicts with other apps | Medium | Low | Configurable; detect common conflicts. |
| Full Access entitlement misunderstanding in App Review | Medium | Medium | Mirror Wispr Flow's justification language; reference precedent in the submission notes. |

---

## Definition of "Shippable MVP"

Checkbox to mark Phase 1 complete:

- [ ] All Phase 1 P0 features work on real hardware (iPhone 16, M1 Mac 16GB).
- [ ] WER ≤ 5% on eval set.
- [ ] Latency p50 ≤ 3s (iOS), ≤ 2s (Mac).
- [ ] Peak RAM ≤ 1.9 GB (iOS), ≤ 5 GB (Mac).
- [ ] Zero required network calls in the dictation path.
- [ ] Zero crashes in 7 days of daily use.
- [ ] Onboarding: mic permission, Accessibility, model download, first successful dictation.
- [ ] Privacy audit green.
- [ ] Release notes + known-issues list drafted.
