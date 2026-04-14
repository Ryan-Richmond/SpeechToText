# Project Vox — Open Questions Resolution

**Document:** PRD v1.0 Addendum — Open Questions
**Date:** 2026-04-08
**Status:** Resolved (pending benchmarks on Q1)

---

## Question 1: Is Gemma 4 E4B viable on iPhone 16 (8GB RAM)?

### Answer: No — E4B is too tight for the iPhone 16. Use E2B for iOS, E4B for Mac.

**The math:**

- iPhone 16 has 8GB total RAM (all models, including Pro)
- iOS itself + background services consume ~2.5–3.5GB at rest
- iOS Jetsam (the kernel memory manager) aggressively kills foreground apps that push total device memory pressure too high. There's no hard published limit, but empirical data shows foreground apps on 8GB devices get reliably killed around 4–5GB of app memory usage. Apple offers an `increased-memory-limit` entitlement that can push this higher for ML workloads, but it's not guaranteed.
- Gemma 4 E4B at Q4_K_M quantization requires **~3–4GB RAM** for model weights + KV cache
- Whisper small.en adds **~250MB**
- App overhead (SwiftUI, audio buffers, SwiftData): **~100–200MB**
- **Total estimated: ~3.5–4.5GB** — this puts us right at the Jetsam kill threshold

**The risk:** Even if it technically loads, any background activity (notifications, music, Maps) could push the device over the edge and kill Vox mid-dictation. That's a terrible user experience and would generate 1-star reviews immediately.

**Gemma 4 E2B is the right choice for iOS:**

- E2B at Q4_K_M: **~1.5GB RAM** for model weights
- Whisper small.en: ~250MB
- App overhead: ~150MB
- **Total estimated: ~1.9GB** — well within safe limits, even with background apps
- E2B still has native audio support, function calling, and 128K context
- The audio encoder is 50% smaller than E4B's, but for clean dictation audio (phone mic, close range), this is fine

**Revised model allocation:**

| Platform | STT Model | Cleanup Model | Total RAM | Headroom |
|----------|-----------|---------------|-----------|----------|
| **iPhone 16+** | Whisper small.en (~250MB) | Gemma 4 E2B Q4_K_M (~1.5GB) | ~1.9GB | Comfortable |
| **M1 Mac 16GB** | Whisper medium.en (~750MB) | Gemma 4 E4B Q4_K_M (~3.5GB) | ~4.5GB | Good |
| **M3 Mac 36GB+** | Whisper large-v3 (~1.5GB) | Gemma 4 E4B Q8_0 (~7GB) | ~9GB | Plenty |

**Minimum iPhone model:** iPhone 15 Pro or later (8GB RAM + A17 Pro with Neural Engine). The standard iPhone 15 has only 6GB and cannot safely run even E2B + Whisper. The iPhone 16 line (all models have 8GB + A18) is the practical minimum for comfortable use.

**What about iPhone 17?** If Apple increases to 12GB RAM (rumored for Pro models), E4B becomes viable on mobile. The architecture should detect available RAM at startup and select the appropriate model tier automatically.

**Decision:** Ship E2B as the iOS default. E4B is Mac-only. Build an adaptive model selector that checks `ProcessInfo.processInfo.physicalMemory` at launch and picks the best model tier.

---

## Question 2: iOS Keyboard Extension vs. Accessibility Overlay

### Answer: Keyboard extension — with a critical workaround that Wispr Flow already proved works.

**The keyboard extension problem:** iOS keyboard extensions are severely memory-constrained (~30–50MB hard limit via Jetsam). You cannot load a 1.5GB AI model inside a keyboard extension. Period.

**How Wispr Flow solved this (and it's been approved by App Store Review):**

The keyboard extension is a **thin shell** that does NOT run inference. Here's their actual architecture:

1. User taps mic button in the keyboard extension
2. Keyboard extension opens the **main Vox app** via deep link (`vox://dictation`)
3. Main app (which has full memory access) captures audio, runs Whisper + Gemma inference
4. Main app copies cleaned text to clipboard (or uses App Group shared container)
5. Main app navigates back to the previous app
6. Keyboard extension pastes the text into the active text field

This is called the "trampoline" pattern. It's the same approach Wispr Flow uses on iOS — a 9to5Mac review confirmed Wispr Flow "takes you to the full-blown Wispr Flow app, activates the Flow Session, and then hops you back to where you were." Users noted it feels a bit fiddly, but it's the only viable approach given iOS limitations, and Apple approves it.

**Key implementation detail:** The return-to-previous-app step is the hardest part. There's no public API for `openPreviousApp()`. Wispr Flow appears to use a combination of:
- Prompting the user to swipe back (iOS 26 behavior)
- Background task completion that triggers a return
- This remains the jankiest part of the iOS experience — a known limitation

**Alternative considered: Full-screen overlay app (no keyboard extension)**
- Skip the keyboard extension entirely
- User activates via Shortcuts/Action Button → Vox app opens → dictate → copy to clipboard → user switches back manually
- Simpler architecture but worse UX (no in-keyboard trigger)
- Could be the MVP approach, with keyboard extension as v1.1

**Decision:** MVP ships with the **Action Button / Shortcuts** approach (no keyboard extension). This is simpler, avoids the trampoline jankiness, and works on iPhone 15 Pro+ and all iPhone 16 models. Add the keyboard extension trampoline in v1.1 once the core inference pipeline is solid.

**Mac approach is simpler:** System-wide hotkey via Accessibility permission → capture audio → process → paste via `NSPasteboard` + simulated Cmd+V. No keyboard extension needed on macOS.

---

## Question 3: Whisper Model Selection — small.en vs. medium.en

### Answer: small.en for iOS, medium.en for Mac, with user-selectable upgrade.

| Model | Size (disk) | RAM | Latency (10s audio, Apple Silicon) | WER (English) |
|-------|------------|-----|-----------------------------------|---------------|
| **tiny.en** | 39MB | ~75MB | ~0.3s | ~7.5% |
| **base.en** | 74MB | ~150MB | ~0.5s | ~5.5% |
| **small.en** | 244MB | ~250MB | ~1.0s | ~4.0% |
| **medium.en** | 769MB | ~750MB | ~1.5s | ~3.5% |
| **large-v3** | 1.55GB | ~1.5GB | ~3.0s | ~3.0% |

**iOS default: small.en**
- 4% WER is excellent — better than most humans can type accurately
- 250MB RAM fits comfortably alongside E2B
- 1 second latency for the STT pass is imperceptible when combined with LLM cleanup
- The LLM cleanup pass will fix many of the remaining 4% errors anyway

**Mac default: medium.en**
- 3.5% WER — marginal improvement but meaningful for professional use
- 750MB RAM is fine on 16GB Mac
- Slightly slower but Mac has more compute headroom

**User setting:** Expose a "Transcription Quality" toggle in Settings:
- "Fast" → tiny.en or base.en (for older devices or low-power mode)
- "Balanced" → small.en (default)
- "Maximum" → medium.en or large-v3 (for power users with RAM to spare)

**Decision:** small.en on iOS, medium.en on Mac, user-adjustable.

---

## Question 4: Core ML Conversion vs. Raw GGUF

### Answer: Start with GGUF via llama.cpp/whisper.cpp. Evaluate Core ML conversion in Phase 2.

**GGUF (via llama.cpp / whisper.cpp):**
- Works today with day-0 Gemma 4 support
- Metal acceleration on Apple Silicon (GPU)
- Mature, battle-tested, huge community
- Single binary for Whisper and Gemma
- Easy to swap models (just download a new GGUF file)

**Core ML (via coremltools conversion):**
- Leverages Apple Neural Engine (ANE) — up to 2–3x faster than GPU-only
- Lower power consumption (ANE is more efficient than GPU)
- Better thermal management (important for sustained mobile use)
- BUT: conversion toolchain is fragile; not all model architectures convert cleanly
- Gemma 4's hybrid attention + PLE + shared KV cache may not convert trivially
- Whisper has well-tested Core ML conversions (whisper-coreml exists)

**Recommendation:**

Phase 1 (MVP): Use **whisper.cpp + llama.cpp** with Metal acceleration. This works today, is proven, and gets us to a working product fastest. Whisper already has excellent Metal performance on Apple Silicon. Gemma E2B/E4B via llama.cpp with Metal gets us good performance.

Phase 2 (optimization): Convert Whisper to Core ML (already proven by existing open-source projects). Investigate Core ML conversion for Gemma E2B/E4B — if it works, the ANE speedup on iPhone would be transformative (2–3x faster, less battery drain). If conversion fails, stay on llama.cpp.

**Decision:** GGUF + llama.cpp for MVP. Core ML conversion is a Phase 2 optimization.

---

## Question 5: App Store Keyboard Extension Review Risk

### Answer: Low risk — Wispr Flow proved the pattern is App Store-approvable.

**Precedent:**
- Wispr Flow is live on the App Store as "Wispr Flow: AI Voice Keyboard" (requires iOS 18.3+)
- It uses the exact keyboard extension → main app trampoline pattern
- It requires Full Access permission for the keyboard
- It requires microphone access
- Apple approved it, and it remains in the store

**Key compliance points for our submission:**
1. **Privacy manifest must be airtight** — declare microphone usage, no tracking, no third-party data sharing
2. **No overclaiming "AI"** — App Store Review is sensitive to vague AI claims. Be specific: "on-device voice dictation with AI text cleanup"
3. **Full Access justification** — Apple requires you to explain why your keyboard needs Full Access. Our reason: microphone access for dictation (legitimate, same as Wispr Flow)
4. **Data handling disclosure** — since we process everything on-device, this is actually a competitive advantage in App Review. We can truthfully say "no audio data leaves the device"

**Risk mitigation:**
- If we ship MVP without a keyboard extension (Action Button approach), this question becomes moot for v1
- When we add the keyboard extension in v1.1, we have Wispr Flow's approval as direct precedent

**Decision:** Low risk. Proceed with confidence. Ship MVP without keyboard extension; add it in v1.1.

---

## Question 6: Naming / Trademark

### Answer: Defer until pre-submission. Use "Vox" as codename for development.

**Quick assessment:**
- "Vox" is a common word (Latin for "voice") — likely not trademarkable on its own in the dictation category
- Vox Media exists as a major media company — potential conflict
- Several "Vox" apps exist on the App Store

**Name alternatives to evaluate before submission:**
- **Vox** — simple but potentially conflicting
- **Diction** — clean, professional, descriptive
- **Murmur** — unique, evokes whisper-mode dictation
- **Parlance** — sophisticated, means "way of speaking"
- **Cadence** — fits the Meridian portfolio aesthetic

**Action:** Run trademark search + App Store name search before committing. For now, "Vox" works as the internal codename.

---

## Summary: Updated Architecture

Based on these answers, the revised Vox architecture is:

```
┌─────────────────────────────────────────────────────┐
│                    VOX ARCHITECTURE                   │
├─────────────────────────────────────────────────────┤
│                                                       │
│  iPhone (iOS 18+, iPhone 15 Pro+)                    │
│  ┌─────────────────────────────────────────────┐    │
│  │  Microphone → AVAudioEngine                   │    │
│  │    → Whisper small.en (whisper.cpp + Metal)   │    │
│  │    → Gemma 4 E2B Q4_K_M (llama.cpp + Metal)  │    │
│  │    → Clipboard paste / App Group handoff       │    │
│  │  RAM budget: ~1.9GB                            │    │
│  └─────────────────────────────────────────────┘    │
│                                                       │
│  Mac (macOS 15+, M1 16GB+)                           │
│  ┌─────────────────────────────────────────────┐    │
│  │  Microphone → AVAudioEngine                   │    │
│  │    → Whisper medium.en (whisper.cpp + Metal)  │    │
│  │    → Gemma 4 E4B Q4_K_M (llama.cpp + Metal)  │    │
│  │    → NSPasteboard + simulated Cmd+V           │    │
│  │  RAM budget: ~4.5GB                            │    │
│  └─────────────────────────────────────────────┘    │
│                                                       │
│  Shared:                                              │
│  • SwiftUI multiplatform UI                           │
│  • SwiftData local storage                            │
│  • App Group shared container (iOS, for future         │
│    keyboard extension)                                │
│  • Adaptive model selector based on device RAM        │
│                                                       │
│  MVP iOS Input: Action Button / Shortcuts             │
│  v1.1 iOS Input: + Keyboard extension (trampoline)    │
│  Mac Input: Global hotkey (Accessibility permission)  │
│                                                       │
│  Inference: GGUF + llama.cpp/whisper.cpp (Phase 1)   │
│  Optimization: Core ML conversion (Phase 2)           │
│                                                       │
└─────────────────────────────────────────────────────┘
```

### Minimum Hardware Requirements

| Device | Minimum | Recommended |
|--------|---------|-------------|
| **iPhone** | iPhone 15 Pro (8GB, A17 Pro) | iPhone 16 (8GB, A18) |
| **Mac** | M1 MacBook Air 16GB | M1 MacBook Pro 16GB+ |

### Model Download Sizes (First Launch)

| Platform | Whisper | Gemma | Total Download |
|----------|---------|-------|----------------|
| **iPhone** | small.en (244MB) | E2B Q4_K_M (~1GB) | ~1.25GB |
| **Mac** | medium.en (769MB) | E4B Q4_K_M (~3GB) | ~3.75GB |

---

## Next Steps

1. **Set up Xcode project** with SwiftUI multiplatform target (iOS + macOS)
2. **Integrate whisper.cpp** via Swift Package Manager
3. **Integrate llama.cpp** via Swift Package Manager
4. **Build proof-of-concept**: record → Whisper → Gemma → paste (Mac first, easier to debug)
5. **Benchmark E2B vs. E4B** on target devices to validate RAM/latency estimates
6. **Write the cleanup system prompt** and iterate on quality
