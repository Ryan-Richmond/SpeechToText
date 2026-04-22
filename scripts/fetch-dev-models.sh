#!/usr/bin/env bash
# fetch-dev-models.sh
#
# Developer convenience: download the model weights for the current device's
# tier into ./.dev-models/ for local Swift development and benchmarking.
#
# IMPORTANT: This is a DEVELOPER tool only. In production, ModelDownloadService
# downloads on first launch into the App Group container. Never commit weights
# to the repo. (.dev-models/ is in .gitignore — keep it that way.)
#
# Usage:
#   bash scripts/fetch-dev-models.sh                  # auto-detect tier from RAM
#   bash scripts/fetch-dev-models.sh --tier ios
#   bash scripts/fetch-dev-models.sh --tier mac-default
#   bash scripts/fetch-dev-models.sh --tier mac-power
#   bash scripts/fetch-dev-models.sh --model gemma-4-E2B-Q4_K_M  # one model
#   bash scripts/fetch-dev-models.sh --list           # list available models

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REGISTRY="$ROOT/Vox/Resources/Models/registry.json"
DEST="$ROOT/.dev-models"

if [[ ! -f "$REGISTRY" ]]; then
    echo "error: registry not found at $REGISTRY" >&2
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "error: jq is required (brew install jq)" >&2
    exit 2
fi

if ! command -v curl >/dev/null 2>&1; then
    echo "error: curl is required" >&2
    exit 2
fi

# ── Argument parsing ──────────────────────────────────────────────────────────
TIER=""
MODEL=""
LIST=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --tier)  TIER="$2";  shift 2 ;;
        --model) MODEL="$2"; shift 2 ;;
        --list)  LIST=true;  shift ;;
        -h|--help)
            sed -n '/^# Usage/,/^$/p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *) echo "error: unknown arg $1" >&2; exit 2 ;;
    esac
done

# ── --list ────────────────────────────────────────────────────────────────────
if $LIST; then
    echo "Available models:"
    jq -r '.models | to_entries[] | "  \(.key)  (\(.value.kind), \(.value.size_bytes | . / 1073741824 | . * 10 | floor / 10) GB)"' "$REGISTRY"
    echo ""
    echo "Tiers:"
    jq -r '.tiers | to_entries[] | "  \(.key): stt=\(.value.stt), llm=\(.value.llm)"' "$REGISTRY"
    exit 0
fi

# ── Auto-detect tier from physical memory ─────────────────────────────────────
detect_tier() {
    local ram_gb
    case "$(uname -s)" in
        Darwin) ram_gb=$(( $(sysctl -n hw.memsize) / 1073741824 )) ;;
        Linux)  ram_gb=$(( $(grep MemTotal /proc/meminfo | awk '{print $2}') / 1048576 )) ;;
        *)      ram_gb=16 ;;
    esac
    echo "[fetch-dev-models] Detected ${ram_gb} GB RAM" >&2
    if   [[ $ram_gb -ge 32 ]]; then echo "mac-power"
    elif [[ $ram_gb -ge 16 ]]; then echo "mac-default"
    else echo "ios"
    fi
}

if [[ -z "$TIER" && -z "$MODEL" ]]; then
    TIER=$(detect_tier)
    echo "[fetch-dev-models] Using tier: $TIER (override with --tier)" >&2
fi

# ── Resolve which models to fetch ─────────────────────────────────────────────
declare -a MODELS_TO_FETCH=()

if [[ -n "$MODEL" ]]; then
    MODELS_TO_FETCH=("$MODEL")
elif [[ -n "$TIER" ]]; then
    stt=$(jq -r --arg t "$TIER" '.tiers[$t].stt // empty' "$REGISTRY")
    llm=$(jq -r --arg t "$TIER" '.tiers[$t].llm // empty' "$REGISTRY")
    if [[ -z "$stt" || -z "$llm" ]]; then
        echo "error: tier '$TIER' not found in registry" >&2
        echo "Run with --list to see available tiers." >&2
        exit 1
    fi
    MODELS_TO_FETCH=("$stt" "$llm")
fi

mkdir -p "$DEST"

# ── Fetch loop ────────────────────────────────────────────────────────────────
fetch_one() {
    local id="$1"
    local entry url filename size_bytes sha256
    entry=$(jq -r --arg id "$id" '.models[$id] // empty' "$REGISTRY")
    if [[ -z "$entry" || "$entry" == "null" ]]; then
        echo "error: model '$id' not in registry" >&2
        return 1
    fi
    url=$(jq -r '.url' <<<"$entry")
    filename=$(jq -r '.filename' <<<"$entry")
    size_bytes=$(jq -r '.size_bytes' <<<"$entry")
    sha256=$(jq -r '.sha256' <<<"$entry")
    local target="$DEST/$filename"
    local size_gb
    size_gb=$(awk "BEGIN { printf \"%.2f\", $size_bytes / 1073741824 }")

    if [[ -f "$target" ]]; then
        echo "[skip]  $filename already present (${size_gb} GB)"
        return 0
    fi

    echo "[fetch] $filename (~${size_gb} GB)"
    echo "        from $url"
    # Resumable download
    curl -L --fail --progress-bar \
        --continue-at - \
        -o "$target.partial" \
        "$url"
    mv "$target.partial" "$target"

    # SHA-256 verify if registry has one
    if [[ "$sha256" != "TBD" && -n "$sha256" ]]; then
        local actual
        if command -v shasum >/dev/null 2>&1; then
            actual=$(shasum -a 256 "$target" | awk '{print $1}')
        else
            actual=$(sha256sum "$target" | awk '{print $1}')
        fi
        if [[ "$actual" != "$sha256" ]]; then
            echo "error: SHA-256 mismatch for $filename" >&2
            echo "  expected: $sha256" >&2
            echo "  got:      $actual" >&2
            mv "$target" "$target.bad"
            return 1
        fi
        echo "[ok]    SHA-256 verified"
    else
        echo "[warn]  SHA-256 is 'TBD' in registry — pin it before relying on this file"
    fi
}

echo "Destination: $DEST"
echo "Models:      ${MODELS_TO_FETCH[*]}"
echo ""

for id in "${MODELS_TO_FETCH[@]}"; do
    fetch_one "$id"
    echo ""
done

echo "Done. Models in $DEST"
echo ""
echo "REMINDER: .dev-models/ is gitignored. Never commit model weights."
