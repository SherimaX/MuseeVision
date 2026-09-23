#!/bin/zsh
# Exports the whole museum to usd/ from the Swift builder (Shared/), then validates it.
#
#   tools/usd-export/export.sh                       # the default clock: 21 June 2026, noon UTC
#   tools/usd-export/export.sh --date 2026-12-21T12:00:00Z
#
# Needs Xcode (the macOS SDK with RealityKit) and the USD tools usdcat and usdchecker, which ship
# with macOS in /usr/bin. Compiles Shared/ plus this folder into a macOS command-line tool, builds
# the museum exactly as the app does, and writes usd/ (README.md there is left alone).
set -euo pipefail

HERE="${0:A:h}"
REPO="${HERE:h:h}"
export DEVELOPER_DIR="${DEVELOPER_DIR:-/Applications/Xcode-beta.app/Contents/Developer}"
WORK="${TMPDIR:-/tmp}/museum-usd"
mkdir -p "$WORK"

echo "Compiling museum-usd…"
xcrun --sdk macosx swiftc -O -swift-version 5 -parse-as-library \
  "$REPO"/Shared/*.swift "$REPO"/Shared/Core/*.swift "$REPO"/Shared/Plan/*.swift "$REPO"/Shared/Wings/*.swift \
  "$HERE"/*.swift \
  -o "$WORK/museum-usd"

"$WORK/museum-usd" --repo "$REPO" --out "$REPO/usd" --stage "$WORK/stage" "$@"

echo "Checking with usdchecker…"
usdchecker "$REPO/usd/museum.usda"
