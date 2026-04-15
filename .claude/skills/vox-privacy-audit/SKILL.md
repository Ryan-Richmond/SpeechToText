---
name: vox-privacy-audit
description: Audit the Vox codebase for anything that would violate the privacy posture — network calls on the dictation path, analytics SDKs, crash-reporting SDKs, PII in logs, tracking identifiers in Info.plist. Fails loudly. Run before every PR and before every release. Safe to run any time; read-only.
---

# vox-privacy-audit

Vox's product promise is "no audio or text leaves the device." This skill is the automated enforcement of that promise.

## When to run

- **Always** before opening a PR that touches `Vox/Pipeline/`, `Vox/Services/`, or any new SPM dependency.
- As the last step of a release checklist.
- Any time you see a new `import` you don't recognize.

## What it checks

The skill runs `scripts/audit.sh` which greps for red flags and summarizes them. Any finding counts as a failure unless explicitly allow-listed in `.vox-privacy-audit.allow`.

**Red flags:**

1. **Network APIs in the dictation path**
   - `URLSession`, `URLRequest`, `NWConnection`, `CFNetwork`, `NSURLConnection`, `AsyncHTTPClient`, `Alamofire`, `URLDownload`
   - Allowed **only** in `Vox/Services/ModelDownloadService.swift` and `Vox/Pipeline/CloudLLMEngine.swift` (BYOK, Phase 2).
2. **Known analytics / crash SDKs**
   - `Firebase`, `Crashlytics`, `Mixpanel`, `Amplitude`, `Segment`, `Sentry`, `Bugsnag`, `AppsFlyer`, `Adjust`, `TelemetryDeck`, `PostHog`, `Datadog`.
3. **PII in logs**
   - `os.Logger` / `print` / `NSLog` calls that interpolate variables named `transcript`, `rawText`, `cleaned`, `audio`, `pcm`, `prompt`, `selection`, `apiKey`, `token`.
4. **Info.plist tracking keys**
   - `NSUserTrackingUsageDescription` (we never want App Tracking Transparency).
   - Any `NSAdvertisingAttributionReportEndpoint`.
5. **Clipboard persistence**
   - `UserDefaults` writes keyed `"clipboard"`, `"lastAudio"`, `"rawTranscript"`.

## How to run

```bash
bash .claude/skills/vox-privacy-audit/scripts/audit.sh
```

Exit code 0 = clean. Non-zero = findings to review.

## How to respond to findings

1. If the finding is legitimate (e.g. a new `URLSession` call in `ModelDownloadService`), add an entry to `.vox-privacy-audit.allow` with a justification comment.
2. If the finding is unintentional, remove the offending code or ask the user for direction.
3. **Never** silence the audit with a blanket `# noqa` or by weakening the grep — that defeats the purpose.

## Output

The script prints a summary of the form:

```
== vox-privacy-audit ==
[PASS] Network APIs (outside allow-list): 0
[PASS] Analytics SDKs: 0
[FAIL] PII in logs: 2
  Vox/Pipeline/PipelineActor.swift:42: logger.info("cleaned: \(cleaned)")
  Vox/Pipeline/PipelineActor.swift:58: print("prompt=\(prompt)")
[PASS] Info.plist tracking keys: 0
[PASS] Clipboard persistence: 0

Result: FAIL (1 category)
```

If running in CI, propagate the non-zero exit code.
