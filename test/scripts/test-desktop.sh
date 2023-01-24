#!/usr/bin/env bash

declare root=""
declare TMPDIR="${TMPDIR:-${TMP:-/tmp}}"

root="$(dirname "$(dirname "${BASH_SOURCE[0]}")")"
rm -rf "$TMPDIR/ssc-socket-test-fixtures"
cp -rf "$root/fixtures/" "$TMPDIR/ssc-socket-test-fixtures"

if [ -z "$DEBUG" ]; then
  ssc build --headless --prod -r -o .
else
  ssc build -r -o .
fi

# rm -rf "$TMPDIR/ssc-socket-test-fixtures"
