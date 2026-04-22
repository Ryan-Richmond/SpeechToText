#!/usr/bin/env bash
# vox-entitlement-diff: diff entitlements + Info.plist against last git tag.
# Exit 0 = no changes, 1 = changes detected.

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
ACCEPT_FILE="$ROOT/.vox-entitlement-diff.accepted"

cd "$ROOT"

echo "== vox-entitlement-diff =="

# ── Find reference tag ─────────────────────────────────────────────────────────
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || true)

if [[ -z "$LAST_TAG" ]]; then
    echo "No git tags found. Cannot diff — this is expected on a fresh repo."
    echo "Result: NO BASELINE (first release; review all entitlements manually)"
    exit 0
fi

echo "Comparing HEAD → $LAST_TAG"
echo ""

# ── Collect files to diff ──────────────────────────────────────────────────────
PATTERNS=("*.entitlements" "*/Info.plist")
FILES_FOUND=0
CHANGES_FOUND=0

for pat in "${PATTERNS[@]}"; do
    # Files present in HEAD
    while IFS= read -r -d '' f; do
        rel="${f#$ROOT/}"

        # Check if the file existed at the tag
        if ! git show "${LAST_TAG}:${rel}" &>/dev/null; then
            echo "NEW FILE: $rel (not present in $LAST_TAG)"
            CHANGES_FOUND=1
            FILES_FOUND=1
            continue
        fi

        FILES_FOUND=1
        DIFF=$(git diff "${LAST_TAG}" -- "$rel" 2>/dev/null || true)
        if [[ -z "$DIFF" ]]; then
            echo "$rel: (no change)"
            continue
        fi

        # Filter accepted entries
        FILTERED_DIFF=""
        if [[ -f "$ACCEPT_FILE" ]]; then
            while IFS= read -r dline; do
                accepted=false
                while IFS= read -r apat; do
                    [[ "$apat" =~ ^[[:space:]]*# ]] && continue
                    [[ -z "${apat// }" ]] && continue
                    [[ "$dline" == *"$apat"* ]] && { accepted=true; break; }
                done < <(grep -v '^\s*#' "$ACCEPT_FILE" | grep -v '^\s*$')
                $accepted || FILTERED_DIFF+="$dline"$'\n'
            done <<< "$DIFF"
        else
            FILTERED_DIFF="$DIFF"
        fi

        if [[ -z "${FILTERED_DIFF// }" ]]; then
            echo "$rel: (changes accepted in .vox-entitlement-diff.accepted)"
        else
            echo "$rel:"
            # Highlight dangerous keys
            echo "$FILTERED_DIFF" | while IFS= read -r dline; do
                if [[ "$dline" == *"NSUserTrackingUsageDescription"* ]]; then
                    echo "  $dline  ← WARNING: tracking permission"
                elif [[ "$dline" == *"NSAdvertisingAttributionReportEndpoint"* ]]; then
                    echo "  $dline  ← WARNING: attribution endpoint"
                else
                    echo "  $dline"
                fi
            done
            CHANGES_FOUND=1
        fi

    done < <(find "$ROOT" -name "$(basename "$pat")" -not -path '*/.git/*' -not -path '*/build/*' -not -path '*/.build/*' -print0 2>/dev/null)
done

if [[ $FILES_FOUND -eq 0 ]]; then
    echo "No .entitlements or Info.plist files found (pre-Sprint 0). Nothing to diff."
    exit 0
fi

echo ""
if [[ $CHANGES_FOUND -eq 0 ]]; then
    echo "Result: NO CHANGES"
    exit 0
else
    echo "Result: CHANGES DETECTED — review before proceeding"
    exit 1
fi
