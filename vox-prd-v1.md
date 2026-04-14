# Project Vox — Product Requirements Document

**Codename:** Vox (working title — Meridian LLC)
**Version:** PRD v1.0
**Status:** Draft
**Author:** Rodge
**Last Updated:** 2026-04-08
**Platform:** iOS 18+ / macOS 15+ (Sequoia) — SwiftUI multiplatform
**License:** Proprietary (Meridian LLC) — built on Apache 2.0 open-source models

---

## 1. Executive Summary

Vox is a privacy-first, offline-capable voice dictation app that transforms natural speech into polished, context-aware text — directly into any text field on iOS and macOS. Unlike Wispr Flow ($15/mo, cloud-only), Vox runs entirely on-device using open-source AI models, eliminating API costs, cloud dependency, and privacy risk. The core pipeline pairs a speech-to-text engine with Gemma 4's on-device language model for real-time cleanup, formatting, and voice commands.

**Target user:** Knowledge workers, developers, content creators, and accessibility-focused users who value speed, privacy, and zero recurring cost.

**Key differentiator:** Full Wispr Flow feature parity with zero cloud dependency, zero subscription, and zero data leaving the device.

---

## 2. Codename Options

| Name | Rationale | Domain Check |
|------|-----------|-------------|
| **Vox** | Latin for "voice" — short, memorable, strong brand | TBD |
| **Diction** | Direct reference to dictation — professional feel | TBD |
| **Murmur** | Evokes quiet/whisper dictation — unique positioning | TBD |

**Recommendation:** Vox — it's punchy, universal, and works for both consumer and enterprise branding. Use "Vox" as the codename and evaluate final naming before App Store submission.

---

## 3. Goals & Non-Goals

### Goals

1. Deliver Wispr Flow-equivalent voice dictation quality using 100% on-device processing
2. Run on iPhone 16+ and M1 Mac (16GB) without perceptible performance degradation
3. Achieve < 2 second end-to-end latency (speech → polished text at cursor) for typical utterances (< 30 seconds)
4. Support Command Mode ("make this friendlier," "turn this into bullets") via voice
5. Ship as a single SwiftUI codebase targeting iOS + macOS simultaneously
6. Maintain zero recurring infrastructure cost for the user

### Non-Goals

- Android/Windows support (Phase 2 — Electron port)
- Real-time streaming transcription (batch-after-utterance is acceptable for MVP)
- Voice cloning, TTS, or audio output
- Meeting transcription / multi-speaker diarization
- Cloud sync of transcription history (local-only for v1)
- Custom model fine-tuning UI for end users

---

## 4. User Personas

### Persona 1: "Alex" — Knowledge Worker

- **Role:** Product manager, heavy email/Slack/Docs user
- **Jobs-to-be-done:** Dictate emails, Slack messages, meeting follow-ups at 3–4x typing speed
- **Pain points:** Wispr Flow subscription cost ($180/yr); privacy concerns with cloud processing; unreliable on poor WiFi
- **Success metric:** Can dictate and send a polished email without touching the keyboard

### Persona 2: "Sam" — Developer

- **Role:** Software engineer using Cursor/VS Code
- **Jobs-to-be-done:** Dictate code comments, commit messages, documentation, and Claude/ChatGPT prompts
- **Pain points:** Context-switching from keyboard to voice tool; dictation tools don't understand code syntax
- **Success metric:** Can dictate a PR description that includes correct technical terms and formatting

### Persona 3: "Jordan" — Accessibility User

- **Role:** Professional with RSI, Parkinson's, or mobility impairment
- **Jobs-to-be-done:** Replace keyboard input for all text entry
- **Pain points:** Built-in dictation lacks cleanup; cloud tools require constant internet; subscription adds financial burden
- **Success metric:** Can complete a full workday with minimal keyboard interaction

---

## 5. Architecture Decision: Pipeline Comparison

### Option A: Two-Stage Pipeline (Whisper + Gemma LLM)

```
Microphone → Audio Buffer
  → Stage 1: Whisper (whisper.cpp / Core ML) → raw transcript
  → Stage 2: Gemma 4 E4B (llama.cpp / Core ML) → polished text
  → Paste at cursor
```

**Pros:**
- Whisper is the gold standard for STT accuracy — battle-tested, massive community
- Each component can be upgraded independently
- Whisper models are small (39MB–1.5GB) and fast
- Gemma E4B can focus entirely on text cleanup/formatting (its strength)

**Cons:**
- Two models in memory simultaneously
- Slightly higher total latency (STT + LLM passes)
- More complex pipeline to maintain

**Estimated resource usage (iPhone 16):**
- Whisper small.en (Core ML): ~250MB RAM, ~1s for 10s audio
- Gemma 4 E4B Q4_K_M: ~3–4GB RAM, ~1–2s for cleanup pass
- **Total: ~4GB RAM, ~2–3s latency**

**Estimated resource usage (M1 Mac 16GB):**
- Whisper medium.en (Core ML): ~750MB RAM, ~0.5s for 10s audio
- Gemma 4 E4B Q4_K_M: ~3–4GB RAM, ~0.5–1s for cleanup pass
- **Total: ~5GB RAM, ~1–1.5s latency**

### Option B: Single-Stage Pipeline (Gemma 4 E4B End-to-End)

```
Microphone → Audio Buffer
  → Gemma 4 E4B (audio encoder + text decoder) → polished text
  → Paste at cursor
```

**Pros:**
- Single model in memory — simpler architecture
- Gemma E4B's audio encoder was designed for exactly this use case
- Fewer moving parts = fewer failure modes
- Could theoretically produce cleanup and transcription in a single pass

**Cons:**
- Audio encoder is 50% smaller than Gemma 3N's — optimized for edge, not accuracy
- Maximum 30 seconds of audio input per inference call
- Less accurate in noisy environments and with accented speech compared to dedicated Whisper
- Text cleanup quality and STT quality are coupled — can't upgrade one without the other
- Less mature ecosystem vs. Whisper (which has 5+ years of community optimization)
- If STT accuracy is poor, the LLM cleanup pass can't fix what it doesn't have

**Estimated resource usage (iPhone 16):**
- Gemma 4 E4B Q4_K_M: ~3–4GB RAM, ~2–3s for combined STT + cleanup
- **Total: ~4GB RAM, ~2–3s latency**

**Estimated resource usage (M1 Mac 16GB):**
- Gemma 4 E4B Q4_K_M: ~3–4GB RAM, ~1–2s
- **Total: ~4GB RAM, ~1–2s latency**

### ⭐ Recommendation: Option A (Two-Stage) with Option B as Experimental Mode

**Rationale:**

1. **Accuracy is the product.** If the transcription is wrong, no amount of LLM polish fixes it. Whisper's STT accuracy is measurably superior to Gemma E4B's integrated audio encoder, especially in real-world conditions (background noise, accents, domain jargon).

2. **The RAM delta is marginal.** Option A uses ~1GB more on Mac, nearly identical on iPhone. On a 16GB Mac, 5GB is well within budget.

3. **Decoupled upgrades are strategic.** When Whisper v4 or a better local STT drops, we swap Stage 1 without touching Stage 2. When Gemma 5 ships, we swap Stage 2 without touching Stage 1.

4. **Option B is a great "Lite Mode."** For users on constrained devices (older iPhones, 8GB Macs), offer a single-model mode that trades some accuracy for lower RAM. This becomes a settings toggle, not a product fork.

**Build plan:**
- MVP: Two-stage pipeline (Whisper + Gemma E4B)
- v1.1: Add "Lite Mode" toggle that uses Gemma E4B end-to-end
- Benchmark both during development and let data drive the default

---

## 6. Model Selection: Local vs. Larger Local vs. Cloud

### 6A. Local Model Tiers

| Model | Use | RAM (Q4) | Latency (10s audio) | Quality |
|-------|-----|----------|---------------------|---------|
| **Gemma 4 E2B** | iPhone (Lite Mode) | ~1.5GB | ~2–3s | Good — basic cleanup |
| **Gemma 4 E4B** | iPhone + Mac (default) | ~3–4GB | ~1–2s | Very good — full cleanup + commands |
| **Gemma 4 26B A4B** | Mac (Power Mode) | ~10–14GB | ~1–3s | Excellent — nuanced tone matching |

**Recommendation:** Ship E4B as the universal default. The 26B A4B is only worth offering if benchmarks show a meaningful quality gap on Command Mode / tone-matching tasks. Given that it consumes 10–14GB RAM on an M1 16GB (leaving only 2–6GB for the OS + other apps), it's not viable as a default — it would make the Mac feel sluggish.

**Decision rule:** If E4B scores ≥ 85% of 26B quality on our evaluation rubric (filler removal, self-correction, formatting, command execution), ship E4B only. If the gap is > 15%, offer 26B as an opt-in "Power Mode" for 32GB+ Macs.

### 6B. Cloud Model Comparison Scenarios

For context on what we're leaving on the table (and what it would cost), here are cloud alternatives:

#### Scenario: 1,000 dictations/month, average 200 words each

**Assumptions:**
- Average dictation: ~200 words spoken → ~300 tokens input (transcript + system prompt) + ~250 tokens output (cleaned text)
- 1,000 dictations/month = 300K input tokens + 250K output tokens/month
- Command Mode adds ~20% more calls (1,200 total/month → 360K in + 300K out)

| Cloud Model | Input Cost | Output Cost | Monthly Total | Annual Total | Quality vs. E4B |
|---|---|---|---|---|---|
| **Gemini 2.5 Flash-Lite** | $0.10/1M in | $0.40/1M out | $0.16 | $1.87 | Comparable to E4B |
| **Gemini 2.5 Flash** | $0.30/1M in | $2.50/1M out | $0.86 | $10.30 | Slightly better |
| **Gemini 3.1 Flash-Lite** | $0.25/1M in | $1.50/1M out | $0.54 | $6.48 | Better reasoning |
| **GPT-4o mini** | $0.15/1M in | $0.60/1M out | $0.23 | $2.78 | Comparable |
| **GPT-4o** | $2.50/1M in | $10.00/1M out | $3.90 | $46.80 | Significantly better |
| **Claude Haiku 4.5** | $0.25/1M in | $1.25/1M out | $0.47 | $5.63 | Better |

#### Scenario: Power User — 5,000 dictations/month

| Cloud Model | Monthly Total | Annual Total |
|---|---|---|
| **Gemini 2.5 Flash-Lite** | $0.78 | $9.36 |
| **Gemini 2.5 Flash** | $4.30 | $51.60 |
| **GPT-4o mini** | $1.17 | $14.04 |
| **GPT-4o** | $19.50 | $234.00 |

#### Scenario: Enterprise — 50 users × 3,000 dictations/month each

| Cloud Model | Monthly Total | Annual Total |
|---|---|---|
| **Gemini 2.5 Flash-Lite** | $23.40 | $280.80 |
| **Gemini 2.5 Flash** | $129.00 | $1,548.00 |
| **GPT-4o** | $585.00 | $7,020.00 |

### 6C. Analysis & Recommendation

**Key insight:** Cloud costs are surprisingly low for individual users. Gemini 2.5 Flash-Lite at $1.87/year is essentially free. The value proposition of local models is NOT primarily cost savings — it's:

1. **Privacy:** No audio or text ever leaves the device. Critical for HIPAA, legal, government, and security-conscious users (your NYSBOE audience knows this pain).
2. **Offline:** Works on planes, in dead zones, in classified facilities.
3. **Latency consistency:** No network variability. Same speed every time.
4. **No vendor lock-in:** No API key, no rate limits, no deprecation risk.
5. **Zero marginal cost at scale:** An enterprise with 500 users pays $0 incremental.

**Recommendation for Vox architecture:**

```
Default:    Local-only (Whisper + Gemma E4B) — zero cost, full privacy
Optional:   Cloud fallback toggle (bring-your-own-API-key) — for users who want 
            cloud quality on older hardware. Support Gemini Flash-Lite and OpenAI 
            as BYOK options in Settings.
```

This mirrors the OpenWhispr model: free local-first, optional cloud BYOK. It's the right architecture for a product that will start as personal and scale to enterprise.

---

## 7. Core Features & Requirements

### Feature: Global Dictation
**Priority:** P0 (MVP)
**User Story:** As a knowledge worker, I want to press a hotkey (Mac) or tap a button (iOS) and have my speech converted to polished text at my cursor position, so that I can write 3–4x faster than typing.

**Acceptance Criteria:**
- [ ] Hotkey activates dictation from any app (Mac: configurable, default Fn Fn)
- [ ] iOS: Custom keyboard extension with mic button, or floating bubble overlay
- [ ] Audio captured until user releases hotkey / taps stop
- [ ] Raw transcript → LLM cleanup → paste at cursor within 3 seconds
- [ ] Works with zero internet connection

**Apple Platform Notes:**
- macOS: Requires Accessibility permissions for global hotkey + paste simulation
- iOS: Custom keyboard requires `RequestsOpenAccess` entitlement; alternatively, use Accessibility overlay (more complex but better UX)
- Privacy manifest: Microphone usage description required

**Edge Cases:**
- No microphone permission → graceful prompt with one-tap Settings redirect
- Very long dictation (> 60s) → chunk into 30s segments, process sequentially
- Silent/empty audio → no paste, subtle UI feedback

---

### Feature: AI Text Cleanup
**Priority:** P0 (MVP)
**User Story:** As a user, I want my rambling speech automatically cleaned up — filler words removed, grammar fixed, punctuation added — so that my text reads as if I typed it carefully.

**Acceptance Criteria:**
- [ ] Removes filler words: "um," "uh," "like," "you know," "so," "actually" (when filler)
- [ ] Adds punctuation from speech patterns (pauses → periods/commas)
- [ ] Fixes grammar and capitalization
- [ ] Handles self-corrections: "meet at 4pm, actually 3pm" → "meet at 3pm"
- [ ] Preserves user's intended meaning — never adds content
- [ ] Formats numbered lists when user says "1... 2... 3..."

**System Prompt (draft for Gemma E4B cleanup pass):**
```
You are a dictation cleanup engine. Transform raw speech transcripts into clean, 
polished text. Rules:
1. Remove all filler words (um, uh, like, you know, so, basically, actually when filler)
2. Add proper punctuation and capitalization
3. Handle self-corrections: keep only the final version
4. Format numbered/bulleted lists when the speaker clearly intends them
5. Never add content the speaker didn't say
6. Never summarize — preserve full meaning and length
7. Output ONLY the cleaned text. No explanations, no markup.
```

---

### Feature: Context-Aware Formatting
**Priority:** P0 (MVP)
**User Story:** As a user, I want the dictation to automatically adapt its formatting based on where I'm typing — professional for email, casual for texts, code-aware for IDEs.

**Acceptance Criteria:**
- [ ] Detects active application context (Mail, Messages, Slack, Xcode, etc.)
- [ ] Adjusts tone: formal (email), casual (iMessage), technical (code editors)
- [ ] Preserves technical terms, variable names, and code syntax when in IDE context
- [ ] macOS: reads active app bundle ID via Accessibility APIs
- [ ] iOS: keyboard extension knows the text field's `textContentType`

---

### Feature: Command Mode
**Priority:** P0 (MVP)
**User Story:** As a user, I want to highlight text and give a voice command like "make this friendlier" or "turn this into bullet points" to have AI rewrite my text in place.

**Acceptance Criteria:**
- [ ] Activation: user selects text + holds Command Mode hotkey + speaks command
- [ ] Supported commands: tone adjustment, format conversion (prose ↔ bullets), summarize, expand, translate
- [ ] Replacement text appears at selection, preserving cursor position
- [ ] Command intent detection: distinguish between "dictate this text" and "do something with existing text"
- [ ] Works on any selected text, not just Vox-dictated text

**Implementation Note:** Command Mode uses a different system prompt that includes the selected text as context and the spoken command as the instruction. This is a standard prompt template swap — no separate model needed.

---

### Feature: Personal Dictionary
**Priority:** P1 (v1.1)
**User Story:** As a user, I want Vox to learn my names, acronyms, and jargon so it transcribes them correctly every time.

**Acceptance Criteria:**
- [ ] Manual add: user types custom words in Settings
- [ ] Auto-learn: when user corrects a transcription, Vox suggests adding to dictionary
- [ ] Dictionary words injected into Whisper prompt as hints
- [ ] Dictionary synced across iOS ↔ Mac via iCloud (Phase 2)

---

### Feature: Voice Snippets
**Priority:** P1 (v1.1)
**User Story:** As a user, I want to create voice shortcuts — say "my calendar link" and have Vox paste my full Calendly URL.

**Acceptance Criteria:**
- [ ] Create snippet: trigger phrase + expansion text
- [ ] Trigger detection: exact match on transcription
- [ ] Expansion: paste full snippet text at cursor
- [ ] Manage via Settings screen

---

### Feature: Tone Matching / Style Learning
**Priority:** P1 (v1.1)
**User Story:** As a user, I want Vox to learn my writing style over time so the cleanup output sounds like me, not like a generic AI.

**Acceptance Criteria:**
- [ ] Capture writing samples from user corrections (what they changed, and to what)
- [ ] Build style embedding: average sentence length, formality level, vocabulary preferences
- [ ] Inject style context into cleanup prompt
- [ ] User can reset style profile

---

### Feature: Whisper Mode (Quiet Dictation)
**Priority:** P2 (Future)
**User Story:** As a user in a shared space, I want to whisper and still get accurate transcription.

**Acceptance Criteria:**
- [ ] Automatic gain adjustment for low-volume speech
- [ ] Whisper-optimized model weights or preprocessing
- [ ] Toggle in Settings or auto-detect

---

## 8. Screens & Navigation

### macOS

```
System Tray Icon (always present)
  ├── Click → Control Panel (floating window)
  │   ├── Dictation History (scrollable list)
  │   ├── Quick Settings (model, hotkey, defaults)
  │   └── Personal Dictionary
  ├── Hotkey → Dictation Overlay (minimal floating pill)
  │   ├── Recording indicator (waveform animation)
  │   ├── Processing indicator (shimmer)
  │   └── Auto-dismiss on paste
  └── Preferences (standard macOS window)
      ├── General (hotkey config, auto-launch, default mode)
      ├── Models (download/manage local models, BYOK API keys)
      ├── Dictionary & Snippets
      ├── Privacy (data retention, analytics opt-in)
      └── About
```

### iOS

```
Launch → Onboarding (first run)
  ├── Microphone permission request
  ├── Keyboard extension install prompt
  └── Quick tutorial (3 screens)

Keyboard Extension:
  ├── Mic button → Recording state → Processing → Insert text
  ├── Command Mode button → Select text → Speak command → Replace
  └── Settings gear → opens main app

Main App:
  ├── History Tab → Transcription list → Detail (copy, re-process, correct)
  ├── Dictionary Tab → Custom words + Snippets
  ├── Settings Tab
  │   ├── Model Management (download E4B, check for updates)
  │   ├── Keyboard Settings
  │   ├── Privacy
  │   └── About
  └── Onboarding replay
```

---

## 9. Apple Platform Considerations

- [x] **Microphone access** — `NSMicrophoneUsageDescription` required
- [x] **Accessibility (macOS)** — needed for global hotkey + clipboard paste simulation
- [x] **Custom keyboard (iOS)** — `RequestsOpenAccess` for mic access in keyboard extension
- [ ] **App Groups** — shared container between main app and keyboard extension for model/dictionary access
- [x] **Privacy manifest** — declare microphone usage, no tracking, no third-party analytics
- [ ] **Background processing** — NOT needed for MVP (dictation is foreground-only)
- [ ] **StoreKit** — if monetized (see Section 11)
- [x] **Core ML** — preferred runtime for Whisper model on Apple Silicon (Neural Engine)
- [x] **Metal** — for llama.cpp / Gemma inference acceleration
- [ ] **App Store Review** — dictation/keyboard apps face extra scrutiny; ensure privacy description is airtight; no "AI" overclaiming in screenshots

**Minimum iOS version:** iOS 18 — required for latest Core ML optimizations and Apple Intelligence coexistence APIs

**Minimum macOS version:** macOS 15 (Sequoia) — required for latest Metal optimizations

---

## 10. Data Model Summary

```
┌──────────────────┐     ┌──────────────────┐
│  Transcription    │     │  DictionaryEntry  │
├──────────────────┤     ├──────────────────┤
│ id: UUID          │     │ id: UUID          │
│ rawTranscript     │     │ word: String      │
│ cleanedText       │     │ phonetic: String? │
│ commandUsed?      │     │ source: enum      │
│ appContext: String │     │ (manual/auto)     │
│ duration: Double  │     │ createdAt: Date   │
│ createdAt: Date   │     └──────────────────┘
│ model: String     │
│ latencyMs: Int    │     ┌──────────────────┐
└──────────────────┘     │  Snippet          │
                          ├──────────────────┤
┌──────────────────┐     │ id: UUID          │
│  StyleProfile     │     │ trigger: String   │
├──────────────────┤     │ expansion: String │
│ id: UUID          │     │ createdAt: Date   │
│ avgSentenceLen    │     └──────────────────┘
│ formalityScore    │
│ vocabFreqMap      │     ┌──────────────────┐
│ corrections: [...]│     │  ModelConfig      │
│ updatedAt: Date   │     ├──────────────────┤
└──────────────────┘     │ id: UUID          │
                          │ name: String      │
                          │ type: enum        │
                          │ (whisper/gemma)   │
                          │ quantization: str │
                          │ sizeBytes: Int64  │
                          │ isDefault: Bool   │
                          │ downloadedAt: Date│
                          └──────────────────┘
```

**Storage:** SwiftData (local-only, no CloudKit for v1)
**Models on disk:** `~/Library/Application Support/Vox/models/` (macOS) or App Group shared container (iOS)

---

## 11. Monetization & Business Model

### Phase 1: Personal Use (No Monetization)

Vox starts as a personal tool for Rodge under Meridian LLC. No App Store distribution.

### Phase 2: App Store Launch

**Recommended model:** Freemium with one-time Pro unlock

| Tier | Price | Includes |
|------|-------|---------|
| **Free** | $0 | Basic dictation (filler removal, punctuation), 50 dictations/day |
| **Pro** | $29.99 one-time | Unlimited dictation, Command Mode, context-aware formatting, dictionary, snippets, style learning |

**Rationale:**
- One-time purchase differentiates from Wispr Flow's $180/yr subscription
- $29.99 is the sweet spot: lower than Superwhisper's $249 lifetime, higher than VoiceInk's $25 (which has no AI cleanup)
- Free tier is generous enough to demonstrate value; daily cap creates natural upgrade pressure
- No recurring revenue risk — models run locally, so there's no marginal cost to support

**Alternative considered:** $4.99/month subscription. Rejected because the core value proposition is "no subscription" — charging a subscription would undermine the positioning.

### Phase 3: Enterprise (Electron Port)

Enterprise pricing via Downstreet Digital for Windows/cross-platform deployment with team features.

---

## 12. MVP Definition & Phasing

### Phase 1 — MVP (Target: 8–10 weeks)

P0 features. Minimum bar for personal daily-driver use.

- Global dictation (hotkey on Mac, keyboard extension on iOS)
- Whisper STT (local, Core ML)
- Gemma 4 E4B cleanup pass (local, llama.cpp + Metal)
- AI text cleanup (filler removal, punctuation, grammar, self-corrections)
- Context-aware formatting (basic app detection)
- Command Mode (rewrite selected text via voice)
- Model download manager (first-launch download of Whisper + Gemma weights)
- Dictation history (local)
- Settings (hotkey config, model selection)

### Phase 2 — v1.1 (Target: +4 weeks)

P1 features. Post-MVP polish.

- Personal dictionary with auto-learn
- Voice snippets
- Tone matching / style learning
- "Lite Mode" toggle (Gemma E4B end-to-end, single model)
- BYOK cloud API fallback (Gemini Flash-Lite, OpenAI)
- Transcription export (copy all, share)

### Phase 3 — v2.0 (Future)

P2 features. Validated if users want them.

- Whisper mode (quiet dictation)
- iCloud sync (dictionary, snippets, style profile across devices)
- Electron port for Windows/Linux (enterprise play)
- Multi-language auto-detect and switch
- Meeting transcription mode (longer recordings, speaker separation)
- MCP server integration (dictate into Claude Code, Cursor, etc.)

---

## 13. Technical Implementation Notes

### Inference Stack

| Component | iOS | macOS |
|-----------|-----|-------|
| **Whisper STT** | whisper.cpp via Core ML (Apple Neural Engine) | whisper.cpp via Core ML or native C++ (Metal) |
| **Gemma E4B** | llama.cpp with Metal acceleration | llama.cpp with Metal acceleration |
| **Model format** | GGUF (Q4_K_M quantization) | GGUF (Q4_K_M default, Q8_0 optional for 32GB+ Macs) |

### Key Libraries / Dependencies

- **whisper.cpp** — C++ Whisper inference, Swift bindings via SPM
- **llama.cpp** — C++ LLM inference for Gemma, Swift bindings via SPM
- **SwiftUI** — UI framework (multiplatform)
- **SwiftData** — local persistence
- **AVFoundation** — audio capture
- **Core ML** (optional) — if converting Whisper to .mlpackage for Neural Engine

### Model Distribution

- Models NOT bundled in app binary (too large for App Store)
- First-launch experience: "Download models" screen with progress bar
- Whisper small.en: ~460MB download
- Gemma E4B Q4_K_M GGUF: ~3GB download
- Total first-launch download: ~3.5GB
- Store in App Group shared container (accessible by keyboard extension)

### Privacy Architecture

- All audio processing is on-device
- No analytics, no telemetry, no crash reporting in v1
- Audio buffers are ephemeral — cleared after transcription
- Transcription history stored locally in SwiftData, user-deletable
- BYOK API keys stored in Keychain (if cloud fallback enabled)

---

## 14. Success Metrics

| Goal | Metric | Target |
|------|--------|--------|
| Core quality | Dictation accuracy (WER on test set) | < 5% word error rate |
| Cleanup quality | % of dictations requiring manual correction | < 15% |
| Latency | End-to-end time (speech → text at cursor) | < 3s (iPhone), < 2s (Mac) |
| Resource usage | Peak RAM during dictation | < 5GB (iPhone), < 6GB (Mac) |
| Command Mode | Command intent classification accuracy | > 90% |
| Adoption (post-launch) | DAU/MAU ratio | > 40% |
| Retention (post-launch) | D7 retention | > 50% |

---

## 15. Open Questions

1. **Gemma E4B vs. E2B for iPhone** — is E4B viable on iPhone 16 (8GB RAM) with iOS overhead? Need to benchmark. If not, fall back to E2B for iOS and E4B for Mac. — Owner: Rodge — Due: Sprint 1

2. **iOS keyboard extension vs. accessibility overlay** — keyboard extension is standard but has limitations (sandboxed, limited mic access). Accessibility overlay (like Wispr Flow on Android) provides better UX but requires Accessibility permission, which Apple scrutinizes. — Owner: Rodge — Due: Sprint 1

3. **Whisper model selection** — small.en vs. medium.en for default. Medium is more accurate but 2x the size and slower. Need latency benchmarks on target devices. — Owner: Rodge — Due: Sprint 1

4. **Core ML conversion vs. raw GGUF** — Core ML models leverage Neural Engine (massive speed gain on Apple Silicon) but require conversion toolchain. GGUF via llama.cpp/whisper.cpp is simpler but GPU-only. Worth the conversion effort? — Owner: Rodge — Due: Sprint 2

5. **App Store keyboard extension review** — Apple has rejected dictation keyboard extensions in the past for privacy concerns. Research precedent (Wispr Flow's iOS app uses a keyboard extension successfully). — Owner: Rodge — Due: Before submission

6. **Naming / trademark** — "Vox" is a common word. Check App Store for conflicts, domain availability, trademark clearance before committing. — Owner: Rodge — Due: Before submission

---

## 16. Out of Scope / Future Considerations

- **Android native app** — Flutter or Kotlin. Deferred until iOS/Mac proven.
- **Windows native** — Electron port is the planned path. Tauri also worth evaluating.
- **Real-time streaming transcription** — would require chunked Whisper inference or a streaming STT model. Deferred.
- **Voice assistant / agent mode** — "Hey Vox, schedule a meeting" — agent integration is a natural extension but out of scope for v1.
- **Fine-tuning Gemma for dictation** — could improve cleanup quality significantly. Deferred until we have enough correction data to fine-tune on.
- **Siri Shortcuts / Shortcuts integration** — natural fit for iOS automation. Phase 3.

---

## Appendix A: Competitive Landscape

| Product | Price | Offline? | Open Source? | Key Limitation |
|---------|-------|----------|-------------|----------------|
| **Wispr Flow** | $15/mo | No | No | Cloud-only, privacy concerns |
| **Superwhisper** | $85/yr or $249 lifetime | Yes | No | Mac-only, no AI cleanup in free tier |
| **OpenWhispr** | Free | Yes | Yes (MIT) | Electron (heavy), no iOS |
| **VoiceInk** | $39 one-time | Yes | Yes | Mac-only, no AI cleanup |
| **Spokenly** | Free (local) | Yes | Partial | Mac-only |
| **Apple Dictation** | Free | Yes | No | No AI cleanup, no command mode |
| **Vox (ours)** | Free / $29.99 Pro | Yes | Built on open models | New entrant |

**Vox's positioning:** The only product that combines full Wispr Flow-class AI cleanup with 100% offline operation, on both iOS and macOS, at a one-time price.

---

## Appendix B: Model Quick Reference

| Model | Total Params | Active Params | RAM (Q4) | Context | Audio? | License |
|-------|-------------|--------------|----------|---------|--------|---------|
| Gemma 4 E2B | 5.1B | 2.3B | ~1.5GB | 128K | Yes | Apache 2.0 |
| Gemma 4 E4B | ~8B (est.) | ~4B | ~3–4GB | 128K | Yes | Apache 2.0 |
| Gemma 4 26B A4B | 26B | 3.8B | ~10–14GB | 256K | No | Apache 2.0 |
| Gemma 4 31B | 31B | 31B | ~19GB | 256K | No | Apache 2.0 |
| Whisper small.en | 244M | 244M | ~250MB | 30s audio | STT only | MIT |
| Whisper medium.en | 769M | 769M | ~750MB | 30s audio | STT only | MIT |
| Whisper large-v3 | 1.55B | 1.55B | ~1.5GB | 30s audio | STT only | MIT |
