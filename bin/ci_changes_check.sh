#!/usr/bin/env bash

git diff --quiet
if [ $? -eq 1 ]; then
    echo "Documentation or TypeScript definitions need update. Please do \`npm run gen\`"
    exit 1
fi
