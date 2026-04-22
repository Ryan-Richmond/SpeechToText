# Vox — Product Roadmap

**Last updated:** 2026-04-14
**Owner:** Rodge (Meridian LLC)
**Horizon:** 12–18 months

This is the *product* roadmap — what we build, in what order, and why. For engineering sprint detail, see [`IMPLEMENTATION_PLAN.md`](IMPLEMENTATION_PLAN.md).

---

## Guiding Principles

1. **Offline-first, always.** Network is never on the critical path.
2. **Ship a delightful narrow slice before broadening.** A great Mac dictation tool beats a mediocre iOS + Mac + Windows launch.
3. **Measure before optimizing.** Every "default" (model tier, quant, latency budget) must be backed by a benchmark we can point to.
4. **Privacy > cleverness.** If a feature requires weakening the privacy story, it gets cut or deferred until we find a private way to do it.
5. **Gemma 4 is an advantage, not a gimmick.** We lean into on-device multimodality where it *actually* improves the product (e.g. Lite Mode, Command Mode), not to put a checkmark on the box.

---

## Themes by Phase

| Phase | Theme | Outcome |
|-------|-------|---------|
| Phase 0 | Foundation | Repo, docs, decisions locked. *(You are here.)* |
| Phase 1 | MVP on Mac + iOS | Personal daily-driver: dictate + cleanup + command + history. |
| Phase 2 | Power features & platform polish | Dictionary, snippets, tone matching, Lite Mode, iOS keyboard trampoline, BYOK. |
| Phase 3 | Platform expansion & ecosystem | Electron / Windows port, iCloud sync, Shortcuts, MCP. |
| Phase 4 | Intelligence layer | On-device style fine-tuning, agent mode, meeting capture. |

---

## Phase 0 — Foundation (current)

**Goal:** Ground truth established so builders don't re-litigate decisions.

- [x] PRD v1 written
- [x] Open questions resolved
- [x] README, CLAUDE.md, AGENTS.md
- [x] Architecture doc
- [x] Roadmap (this document)
- [x] Implementation plan
- [ ] Brand / naming trademark check
- [ ] Evaluation dataset assembled (≥ 50 audio samples with reference transcripts + references for cleanup)

**Exit criteria:** PRD + architecture + plan are approved; a single developer could start Sprint 0 without further clarification.

---

## Phase 1 — MVP (Target: 8–10 weeks)

**Goal:** Vox is good enough for Rodge to use as his daily dictation tool.

### P0 Features

- **Global Dictation**
  - Mac: Configurable global hotkey (default: double-tap Fn).
  - iOS: Action Button trigger + Shortcut entry point (no keyboard extension yet).
- **Whisper STT** (medium.en Mac, small.en iOS) via `whisper.cpp` + Metal.
- **Gemma 4 Cleanup** (E4B Mac, E2B iOS) via `llama.cpp` + Metal.
- **AI Text Cleanup** — fillers, punctuation, grammar, self-correction handling.
- **Context-Aware Formatting** — basic active-app detection on macOS; iOS uses `textContentType` where available.
- **Command Mode** — select + hotkey + speak → LLM rewrite at selection.
- **Model Download Manager** — first-launch, resumable, showing progress + space needed.
- **Dictation History** — scrollable list, per-entry copy / re-process / delete.
- **Settings** — hotkey, model, privacy, clear history.

### Quality Bars

- End-to-end latency ≤ 3s on iPhone 16, ≤ 2s on M1 Mac (p50 over the eval set).
- WER ≤ 5% on the Vox eval set (English, clean room audio).
- < 15% of dictations require manual correction (internal dog-fooding metric).
- Peak RAM: iOS ≤ 1.9 GB, Mac ≤ 5.0 GB.
- Zero required network traffic after model download.

### Non-Goals (explicit)

- No iOS keyboard extension.
- No iCloud sync.
- No BYOK cloud fallback.
- No tone matching or style learning.
- No voice snippets.
- No multi-language.

**Exit criteria:** "I've replaced Wispr Flow with Vox for a week and prefer it" (self-test) + latency/accuracy bars hit.

---

## Phase 2 — v1.1 (Target: +4–6 weeks after Phase 1)

**Goal:** Turn MVP into something we can share with early users.

### P1 Features

- **Personal Dictionary** — manual add + auto-suggest on user corrections. Feeds Whisper as prompt hints.
- **Voice Snippets** — trigger phrase → expansion text.
- **Tone Matching / Style Learning** — build a `StyleProfile` from corrections; inject into cleanup prompt.
- **Lite Mode** — single-model Gemma end-to-end path for constrained devices.
- **BYOK Cloud Fallback** — Gemini 2.5 Flash-Lite and OpenAI as opt-in options; key stored in Keychain.
- **iOS Keyboard Extension** — the "trampoline" pattern (ext → main app → paste).
- **Transcription Export** — select, copy, share sheet.
- **Clear UX polish** — onboarding refinement, empty states, first-run model download copy.

### Quality Bars

- Tone-matched output rated ≥ 4/5 by the user on at least 20 messages.
- Lite Mode: RAM ≤ 1.2 GB on iOS; latency penalty ≤ 20% vs default mode.
- Keyboard extension round-trip ≤ 3.5s on iPhone 16.

### Non-Goals

- No Windows / Linux.
- No team or enterprise features.
- No meeting / diarization.

**Exit criteria:** 10 external beta users, ≥ 50% weekly retention over 4 weeks.

---

## Phase 3 — v2.0 (6–9 months out)

**Goal:** Expand platforms and integrations. Prepare enterprise story.

### Themes

- **Electron / Tauri port for Windows & Linux.** Shared inference via llama.cpp + whisper.cpp (C++ is portable). New UI layer.
- **iCloud sync** for dictionary, snippets, and style profile. **Not** transcription history (still local).
- **Siri Shortcuts + Apple Intelligence coexistence** on iOS / Mac.
- **MCP server integration** — dictate directly into Claude Code, Cursor, Windsurf, Zed via an MCP "dictation" tool.
- **Multi-language auto-detect** — Whisper already supports it; expose as a setting.
- **Whisper Mode (quiet dictation)** — gain-adjusted model variant for library / shared-space use.
- **Meeting mode** — longer recordings, basic speaker separation (offline via pyannote-small or equivalent).

### Quality Bars

- Windows build within 20% latency parity of Mac on equivalent hardware.
- iCloud sync converges within 10s on a well-connected device.
- MCP server passes published MCP compliance suite.

---

## Phase 4 — Intelligence Layer (9–18 months out)

**Goal:** Use accumulated user data (local only) to make Vox dramatically better at *this* user's voice and style.

### Themes

- **On-device LoRA fine-tune** of Gemma on user corrections. Runs overnight / on-charger / on-mains. Training path via MLX or a future Metal-capable llama.cpp extension.
- **Agent mode** — "Vox, schedule a 30-minute meeting with Alex tomorrow afternoon." Integrates with Calendar / Reminders via MCP or native APIs.
- **Meeting transcription with summaries** — speaker diarization + action-item extraction, entirely on-device.
- **Vocabulary graph** — a learned graph of the user's entities (people, projects, products) surfacing in dictation context.

### Quality Bars

- Post-fine-tune WER improvement ≥ 15% vs base model on the user's own eval set.
- Agent mode command accuracy ≥ 85% on a 30-command benchmark.

---

## Out of Scope (Indefinitely)

- Voice cloning / TTS / audio output.
- Cloud-hosted Vox backend (by design).
- Android native app — Electron may cover it; native is deferred.
- Shared / collaborative transcription sessions.

---

## Release Checklist (applied per phase)

Every phase release runs this list:

- [ ] All P* features for the phase meet acceptance criteria
- [ ] Quality bars met (benchmark artifacts linked in release notes)
- [ ] Privacy audit clean (`vox-privacy-audit` skill)
- [ ] Entitlement diff reviewed (`vox-entitlement-diff` skill)
- [ ] Onboarding flow re-tested cold
- [ ] Model download sizes documented
- [ ] RAM + latency numbers updated in README
- [ ] Release notes drafted
- [ ] App Store metadata (Phase 2+) reviewed for compliance
- [ ] TestFlight build tagged

---

## Dependencies & Risks

| Risk | Mitigation |
|------|------------|
| Gemma 4 GGUF revision churn (post-launch) | Pin SHA-256 in `Vox/Resources/Models/registry.json`; bump deliberately. Architecture stays model-agnostic. |
| llama.cpp performance regression on Apple Silicon | Pin to known-good commits; vendor if necessary. |
| Apple rejects keyboard extension | Ship without keyboard extension (MVP already does). Wispr Flow precedent reduces risk. |
| iOS 18 memory limit changes | `DeviceCapabilityService` downgrades tier automatically. |
| Trademark conflict on "Vox" | Naming decision is deferred to pre-submission; codename is disposable. |
| User perceives "on-device = slow" | Surface latency metrics in Settings → About ("avg 1.6s last 100 dictations"). Make the speed visible. |

---

## Success Metrics (cumulative across phases)

| Metric | MVP target | v1.1 target | v2.0 target |
|--------|-----------|-------------|-------------|
| WER | ≤ 5% | ≤ 4% | ≤ 3% |
| Latency (Mac, p50) | ≤ 2s | ≤ 1.5s | ≤ 1s |
| Latency (iOS, p50) | ≤ 3s | ≤ 2.5s | ≤ 2s |
| Manual correction rate | ≤ 15% | ≤ 10% | ≤ 7% |
| D7 retention (post-launch) | — | ≥ 50% | ≥ 60% |
| Pro conversion (post-launch) | — | — | ≥ 5% of MAU |

---

## "Would-Add-Value" Ideas to Evaluate

Captured from the PRD + brainstorming, not yet committed to a phase. Each is a ≤ 2-week scoped experiment.

1. **Smart silence detection** that auto-ends dictation instead of requiring key release.
2. **"Undo dictation" hotkey** — reverts the last paste and restores the clipboard.
3. **Punctuation preview** — ghost-text preview in the overlay before paste.
4. **"What did I say?" replay** — keeps the last 3 raw transcripts in memory for quick debugging.
5. **App-specific prompt overrides** — "in Slack, never use periods; in Mail, always sign off with my name."
6. **Privacy mode LED** — a persistent menu-bar indicator when the mic is live. Reinforces the privacy story.
7. **Offline translation** — say a sentence in Spanish, paste in English, using Gemma's native translation.
8. **Calendar + Mail MCP integrations** — "read me my agenda" / "draft a reply to the last email from X" — only if privacy-clean.
