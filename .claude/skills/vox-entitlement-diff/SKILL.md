---
name: vox-entitlement-diff
description: Diff the current entitlements and Info.plist values against the last git tag. Surfaces any new permissions, privacy-relevant keys, or capability additions so they can be reviewed before a PR or release. Read-only and safe to run anytime.
---

# vox-entitlement-diff

Entitlement creep is a real risk for App Store submissions. Apple scrutinizes every entitlement, especially for keyboard extensions and accessibility-adjacent apps. This skill gives you a diff so changes are never accidental.

## When to run

- Before opening any PR that touches `*.entitlements`, `Info.plist`, or `*.xcconfig`.
- As step 2 of `vox-release-checklist`.
- Any time Xcode warns about a capability change.

## How to run

```bash
bash .claude/skills/vox-entitlement-diff/scripts/diff.sh
```

No arguments needed. The script:
1. Finds the last `git tag` on the current branch.
2. Extracts all `*.entitlements` and `Info.plist` files from that tagged commit.
3. Extracts the same files from the working tree (or `HEAD` if no uncommitted changes).
4. Diffs them and formats the output.
5. Exits 0 if there are no changes, 1 if there are (so it can gate CI).

## Output example

```
== vox-entitlement-diff ==
Comparing HEAD → v0.1.0

Vox/Vox.entitlements:
  + com.apple.security.temporary-exception.apple-events = YES
    (NEW — requires justification)

Vox/Info.plist:
  ~ NSMicrophoneUsageDescription:
    - "Vox needs the microphone to capture your speech."
    + "Vox uses the microphone to capture your speech for on-device transcription. No audio leaves your device."
    (CHANGED — review copy for accuracy)

  + NSUserTrackingUsageDescription = "..."
    (NEW — WARNING: tracking permission detected. Vox should never request this.)

VoxKeyboard/VoxKeyboard.entitlements: (no change)

Result: CHANGES DETECTED — review before proceeding
```

## How to respond to findings

| Finding type | Action |
|---|---|
| New entitlement (expected, e.g. App Groups for keyboard ext) | Add a `# justified: <reason>` comment in the entitlements file; accept in the diff |
| New entitlement (unexpected) | Remove it and investigate how it got there |
| `NSUserTrackingUsageDescription` | Immediate blocker — remove, run `vox-privacy-audit` |
| Changed `NSMicrophoneUsageDescription` | Review copy for accuracy and privacy compliance |
| Removed entitlement | Confirm intentional; may break existing users |

## Justification file

Create `.vox-entitlement-diff.accepted` to silence known-good changes across sessions:

```
# Each line is a substring match. Accepted = suppress from diff output.
# Add with justification comment.

# App Groups added in Sprint 9 for keyboard extension handoff
com.apple.security.application-groups
```

This is different from the `vox-privacy-audit` allow-list — that's about code, this is about `plist` and entitlements files.
