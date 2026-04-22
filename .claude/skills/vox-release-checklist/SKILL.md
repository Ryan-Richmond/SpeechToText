---
name: vox-release-checklist
description: Walk through the Vox pre-release checklist interactively. Checks each item, marks pass/fail, and blocks on failures. Run before cutting any release branch. Covers quality bars, privacy, entitlements, onboarding, and App Store compliance.
---

# vox-release-checklist

This skill runs the release checklist from `docs/ROADMAP.md` as an interactive, automated-where-possible gate. Nothing ships without a clean run.

## How to invoke

Just trigger the skill. It will:

1. Determine the current phase from `ROADMAP.md` + git tags.
2. Run every automated check.
3. Pause for manual confirmation on items that can't be automated.
4. Print a signed-off checklist at the end.
5. Write a `docs/releases/checklist-vX.Y-YYYYMMDD.md` artifact you can attach to the GitHub release.

## Checklist (Phase 1 MVP)

### Automated checks

- [ ] `make build` passes for both iOS and macOS schemes.
- [ ] `make test` passes (all unit + integration tests green).
- [ ] `bash .claude/skills/vox-privacy-audit/scripts/audit.sh` exits 0.
- [ ] `bash .claude/skills/vox-entitlement-diff/scripts/diff.sh` — reviewed and accepted.
- [ ] `python3 .claude/skills/vox-prompt-eval/scripts/score.py` ≥ 85% overall pass rate.
- [ ] Latest `docs/benchmarks/model-bench-*.md` meets all targets (WER, latency, RAM).
- [ ] No `TODO(release)` or `FIXME(release)` comments in source.
- [ ] `swiftlint` passes with zero errors.

### Manual checks (pause and confirm each)

- [ ] **Cold install test.** Delete the app, re-install from a clean build, complete onboarding including model download on a real device. Confirm first dictation succeeds within the target latency.
- [ ] **Model download sizes match README.** Open Settings → Models, check displayed sizes against README table.
- [ ] **Onboarding flow.** Mic permission prompt appears correctly. Accessibility walkthrough (macOS) is clear. Model download screen shows progress + estimated size.
- [ ] **Command Mode.** Select text in at least three apps (Mail, Notes, Xcode). Hold hotkey + speak a command. Confirm replacement in all three.
- [ ] **Latency self-test.** Dictate 10 utterances. Subjectively confirm p50 feels within target. Check Settings → About for the displayed average latency.
- [ ] **Privacy mode.** Enable macOS Little Snitch or Charles Proxy. Dictate 5 times. Confirm zero outbound connections.
- [ ] **Permission denial UX.** Revoke Mic permission, attempt dictation, confirm graceful prompt + Settings deep-link. (macOS: revoke Accessibility, confirm the same.)
- [ ] **Long dictation.** Speak for 65+ seconds. Confirm chunked processing completes without crash.
- [ ] **Silent recording.** Activate dictation, stay silent for 5 seconds, stop. Confirm no paste and no crash.
- [ ] **Hotkey conflict.** Attempt the default hotkey in an app that uses the same hotkey (e.g. some apps bind Fn). Confirm graceful handling.
- [ ] **Jetsam simulation (iOS).** Use Xcode Memory Gauge to push memory pressure while Vox is mid-inference. Confirm recovery path surfaces, not a crash.
- [ ] **History.** Dictate 5 items. Confirm history shows all 5. Delete one. Confirm deletion. Copy from history. Confirm clipboard contents.

### App Store compliance (Phase 2+ only)

- [ ] Privacy manifest reviewed — no undeclared data types.
- [ ] No `NSUserTrackingUsageDescription` in `Info.plist`.
- [ ] App Store screenshots don't overclaim ("AI voice dictation" not "sentient AI assistant").
- [ ] `RequestsOpenAccess` justification copy explains mic-only use.
- [ ] Keyboard extension tested on App Store build (not dev-signed).

## How to respond to failures

- **Automated failures:** Fix the issue and re-run the skill before proceeding. Do not skip.
- **Manual failures:** File a GitHub Issue with `[release-blocker]` label. Do not proceed until resolved.
- **Accepted known issues:** Add to `docs/releases/known-issues-vX.Y.md` with a timeline for resolution.

## Output artifact format

```markdown
# Release Checklist — Vox vX.Y — 2026-04-14

## Automated
| Check | Result |
|-------|--------|
| make build (iOS) | PASS |
| make build (macOS) | PASS |
| privacy-audit | PASS |
| ...

## Manual
| Check | Signed off by | Notes |
|-------|--------------|-------|
| Cold install (iPhone 16) | Rodge | Clean install + model download: 4m 32s |
| ...

## Overall: PASS / FAIL
```
