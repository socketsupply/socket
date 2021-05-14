#!/usr/bin/env bash
set -e

# TODO doesn't work on macOS (`sed: unknown primary or operator`)
find . \
    -name 'test.js' \
    -not -path './node_modules/*' \
    -not -path './_test/*' \
    sed s/\.js// |
while read FILE; do
    if ! grep -q "require.*\.${FILE}" test/index.js ; then
        echo "Could not find $FILE" >&2
        exit 1
    fi
done

echo "All tests included!"
