#!/usr/bin/env bash
# new-golden.sh — scaffold a new vox-prompt-eval golden fixture.
# Usage: bash new-golden.sh <id_name>
# Example: bash new-golden.sh 042_self_correction_datetime

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <id_name>  (e.g. 042_self_correction_datetime)"
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
FIXTURE_DIR="$ROOT/VoxTests/Fixtures/cleanup_goldens/$1"

if [[ -d "$FIXTURE_DIR" ]]; then
    echo "error: directory already exists: $FIXTURE_DIR"
    exit 1
fi

mkdir -p "$FIXTURE_DIR"

cat > "$FIXTURE_DIR/input.txt" <<'EOF'
<paste raw Whisper transcript here>
EOF

cat > "$FIXTURE_DIR/context.json" <<'EOF'
{
  "app": "Mail",
  "app_family": "email",
  "dictionary": [],
  "style": null
}
EOF

cat > "$FIXTURE_DIR/reference.txt" <<'EOF'
<paste the ideal cleaned output here>
EOF

echo "Created $FIXTURE_DIR"
echo "  Edit:"
echo "    input.txt      — raw Whisper-style transcript"
echo "    context.json   — app context and personal dictionary hints"
echo "    reference.txt  — the ideal cleaned output"
