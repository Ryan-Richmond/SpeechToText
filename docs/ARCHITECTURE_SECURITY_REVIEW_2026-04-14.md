# Architecture & Security Review (2026-04-14)

## Scope

This review covers the current planning artifacts in a documentation-only repository:

- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/ROADMAP.md`
- `vox-prd-v1.md`
- `vox-open-questions-resolved.md`

No production source code exists yet, so this is a **design review** focused on architecture, security posture,
and long-term maintainability.

---

## Executive Assessment

The planned architecture is strong and aligns with offline-first constraints:

- Clean separation of `Pipeline`, `Features`, `Services`, `Models`, and `UI`.
- Explicit actor boundary (`PipelineActor`) for heavy inference work.
- Platform-specific constraints (iOS keyboard extension memory, macOS accessibility path) are acknowledged early.
- Privacy-first defaults are clearly codified.

The top maintainability/security risks today are mostly **contract and policy gaps** that should be tightened before Sprint 0 implementation.

---

## What is architected well

1. **Good modular decomposition**
   - The proposed module map has sensible boundaries between app orchestration and implementation details.
   - Protocol-led STT/LLM engines make swapability feasible.

2. **Concurrency model is directionally correct**
   - A serial `PipelineActor` should reduce race conditions and lifecycle bugs when loading/unloading large models.

3. **Privacy and product constraints are explicit**
   - "Offline by default" and BYOK-only cloud fallback are well-scoped and user-comprehensible.

4. **Phase discipline**
   - Deferring keyboard extension and cloud fallback to post-MVP reduces early complexity.

---

## Findings (prioritized)

### High

#### 1) Version and compatibility policy is not formally defined
**Why this matters:** Multiple docs reference external engines (`whisper.cpp`, `llama.cpp`) and model variants, but there is no explicit compatibility matrix and pinning policy. This is a supply-chain and reliability risk once implementation begins.

**Risk:** Regressions from upstream changes, silent behavior drift, or build instability.

**Recommendation:**
- Add a `docs/DEPENDENCY_POLICY.md` before Sprint 1 with:
  - Exact pinning strategy (commit SHA/tag) for each native dependency.
  - Update cadence and rollback policy.
  - Model checksum/signature verification requirements.
  - Criteria for vendor/fork decisions.

---

#### 2) Trust boundaries for the iOS trampoline flow are under-specified
**Why this matters:** Keyboard extension ↔ main app handoff via deep link + App Group is security-sensitive.

**Risk:** Data spoofing, stale writes, or accidental cross-request leakage.

**Recommendation:** define a hardened handoff contract now:
- One-time request IDs + short TTL.
- Atomic write/read semantics in App Group container.
- Origin and replay validation.
- Explicit data lifecycle (delete after consumption / timeout).
- Audit log fields that exclude PII.

Document this in `docs/ARCHITECTURE.md` under a dedicated "Trust Boundaries" section.

---

### Medium

#### 3) Inconsistent device minimums across docs
**Observation:** Some docs mention iPhone 15 Pro+ while others emphasize iPhone 16+, which can cause implementation and QA drift.

**Recommendation:** pick a single canonical minimum device matrix and reference it from one source of truth (e.g., `vox-prd-v1.md`), then align all docs.

---

#### 4) Module boundaries are good, but package boundaries are still implicit
**Why this matters:** Without compile-time boundaries, feature code may directly depend on low-level pipeline details over time.

**Recommendation:** For Sprint 0, define package/layer dependency rules:
- `Features` can depend on `Domain` interfaces, not concrete engine implementations.
- `Pipeline` owns engine composition and model lifecycle.
- `Services` expose narrow protocols to features.
- Add architecture tests or lint rules to prevent forbidden imports.

---

#### 5) Security controls are principles-heavy but control-light
**Why this matters:** Principles are clear, but concrete implementation controls are not yet listed.

**Recommendation:** add a checklist document (`docs/SECURITY_BASELINE.md`) with minimum controls:
- Keychain accessibility class for BYOK keys.
- Clipboard handling policy (restore behavior, retention window).
- Redaction schema for logs.
- File-system encryption assumptions for on-device model/data storage.
- Deep link allowlist and parameter validation.

---

### Low

#### 6) Reliability SLOs and failure budgets are missing
**Recommendation:** define MVP SLOs and user-visible fallback behavior:
- STT failure rate target.
- Model load failure recovery path.
- Paste failure fallback UX.

---

## Maintainability Recommendations (modular-first)

Before writing major feature code, add these lightweight artifacts:

1. **Domain contracts package**
   - Create shared interfaces (`DictationUseCase`, `CommandUseCase`, `ModelRepository`) independent of UI and engine wrappers.

2. **Decision records (ADRs)**
   - Short ADRs for major decisions (engine choice, model tiering, keyboard trampoline contract, cloud BYOK policy).

3. **Error taxonomy**
   - Unified error types across capture/STT/LLM/paste so UI handling is deterministic.

4. **Test seams early**
   - Ensure all engine and service dependencies are injectable from day one.

5. **Security regression gate in CI**
   - Add a scripted check for forbidden APIs/patterns (`URLSession` on dictation path, analytics SDK symbols, unsafe log calls).

---

## Suggested Sprint-0 Additions

- `docs/DEPENDENCY_POLICY.md`
- `docs/SECURITY_BASELINE.md`
- `docs/ARCHITECTURE_DECISIONS/` with first 3 ADRs
- A one-page "layer dependency" diagram + rule table

These are small additions that materially reduce architecture drift before code lands.

---

## Final Verdict

The planned architecture is a solid foundation for a modular, maintainable, privacy-preserving product. The most important next step is to convert current high-level principles into **enforceable contracts** (dependency policy, trust boundaries, and security baseline) before Sprint 1 implementation begins.
