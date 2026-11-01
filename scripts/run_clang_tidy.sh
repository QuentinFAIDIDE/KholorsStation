#!/bin/bash

# Runs clang-tidy on the whole src/ folder.
# Usage: scripts/run_clang_tidy.sh [--fix]

# abort on errors
set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"

if [ "$1" == "--fix" ]; then
    # Files are fixed one at a time so that a header shared by several .cpp files
    # is already fixed when the next one is processed (no duplicated edits).
    # Checks whose fixes can break the build or change behaviour (dangling
    # references, enum types, linkage...) stay as warnings only, and the analyzer
    # is skipped since it has no fixes and is slow.
    FILES=$(grep -o '"file": "[^"]*"' "$BUILD_DIR/compile_commands.json" | cut -d'"' -f4 | grep "^$REPO_ROOT/src/" | sort -u)
    for f in $FILES; do
        echo "fixing $f"
        clang-tidy -p "$BUILD_DIR" --quiet --fix \
            -checks='-clang-analyzer-*,-cppcoreguidelines-prefer-member-initializer,-cppcoreguidelines-virtual-class-destructor,-readability-redundant-parentheses,-performance-unnecessary-copy-initialization,-performance-unnecessary-value-param,-performance-enum-size,-misc-use-internal-linkage,-modernize-use-emplace' \
            "$f" > /dev/null 2>&1 || true
    done
else
    run-clang-tidy -p "$BUILD_DIR" -quiet \
        -allow-enabling-alpha-checkers \
        -checks='-clang-analyzer-alpha.*,clang-analyzer-alpha.cplusplus.InvalidatedIterator' \
        -extra-arg=-Xclang -extra-arg=-analyzer-config \
        -extra-arg=-Xclang -extra-arg=aggressive-binary-operation-simplification=true \
        "$REPO_ROOT/src/"
fi
