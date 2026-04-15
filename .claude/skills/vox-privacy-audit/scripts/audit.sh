#!/usr/bin/env bash
# vox-privacy-audit: enforce Vox's no-network / no-PII privacy posture.
# Exit non-zero on any finding outside the allow-list.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
ALLOW_FILE="$ROOT/.vox-privacy-audit.allow"

if ! command -v rg >/dev/null 2>&1; then
    echo "ripgrep (rg) is required." >&2
    exit 2
fi

# Source directories we scan. If Swift sources don't exist yet, the audit still
# runs and reports PASS across the board (there's nothing to audit).
SCAN_DIRS=()
for d in Vox VoxKeyboard VoxTests VoxUITests Packages; do
    [[ -d "$ROOT/$d" ]] && SCAN_DIRS+=("$ROOT/$d")
done

if [[ ${#SCAN_DIRS[@]} -eq 0 ]]; then
    echo "== vox-privacy-audit =="
    echo "No Swift source directories found (pre-Sprint 0). Nothing to audit."
    exit 0
fi

FAIL=0
CATS_FAILED=0

# Allow-list helper: returns 0 if a match line is explicitly allow-listed.
allowed() {
    local line="$1"
    [[ -f "$ALLOW_FILE" ]] || return 1
    # Lines in .vox-privacy-audit.allow are substrings; # starts a comment.
    grep -v '^\s*#' "$ALLOW_FILE" | grep -v '^\s*$' | while read -r pat; do
        [[ "$line" == *"$pat"* ]] && exit 0
    done
    return 1
}

check_category() {
    local name="$1"
    local pattern="$2"
    local glob="$3"
    local matches
    matches=$(rg -n --no-heading -e "$pattern" --glob "$glob" "${SCAN_DIRS[@]}" 2>/dev/null || true)

    local filtered=""
    if [[ -n "$matches" ]]; then
        while IFS= read -r line; do
            if ! allowed "$line"; then
                filtered+="$line"$'\n'
            fi
        done <<< "$matches"
    fi

    if [[ -z "${filtered// }" ]]; then
        printf "[PASS] %s: 0\n" "$name"
    else
        local count
        count=$(printf "%s" "$filtered" | grep -c '')
        printf "[FAIL] %s: %d\n" "$name" "$count"
        printf "%s" "$filtered" | sed 's/^/  /'
        CATS_FAILED=$((CATS_FAILED + 1))
        FAIL=1
    fi
}

echo "== vox-privacy-audit =="

# 1. Network APIs in dictation path.
check_category \
    "Network APIs (outside allow-list)" \
    '\b(URLSession|URLRequest|NWConnection|CFNetwork|NSURLConnection|AsyncHTTPClient|Alamofire|URLDownload)\b' \
    '*.swift'

# 2. Known analytics / crash SDKs.
check_category \
    "Analytics / crash SDKs" \
    '\b(Firebase|Crashlytics|Mixpanel|Amplitude|Segment|Sentry|Bugsnag|AppsFlyer|Adjust|TelemetryDeck|PostHog|Datadog)\b' \
    '*.swift'

# 3. PII in logs (heuristic: logger/print/NSLog interpolating sensitive names).
check_category \
    "PII in logs" \
    '(Logger\.|logger\.|os_log|print\(|NSLog\().*\\\((transcript|rawText|cleaned|audio|pcm|prompt|selection|apiKey|token)' \
    '*.swift'

# 4. Info.plist tracking keys.
check_category \
    "Info.plist tracking keys" \
    'NSUserTrackingUsageDescription|NSAdvertisingAttributionReportEndpoint' \
    '*.plist'

# 5. Clipboard persistence.
check_category \
    "Clipboard / audio persistence keys" \
    'UserDefaults.*(clipboard|lastAudio|rawTranscript)' \
    '*.swift'

echo
if [[ $FAIL -eq 0 ]]; then
    echo "Result: PASS"
    exit 0
else
    echo "Result: FAIL ($CATS_FAILED categor$([[ $CATS_FAILED -eq 1 ]] && echo y || echo ies))"
    exit 1
fi
