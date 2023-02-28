#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare files=($(cd "$root/api" && find *.js **/*.js **/**/*.js 2>/dev/null))
declare length=${#files[@]}
declare output=()

output+=('{')
output+=('  "imports": {')

declare entry="\"socket:modules/\": \"./socket/modules/\""
if (( length > 0 )); then
  entry+=','
fi

output+=("     $entry")

for (( i = 0; i < length; ++i )); do
  declare file="${files[$i]}"
  declare name="${file/.js/}"
  declare entry=''
  declare is_last=$(( i == length - 1 ))

  entry="\"socket:$name\": \"./socket/$file\""

  output+=("     $entry,")

  entry="\"socket:$name.js\": \"./socket/$file\""
  if (( ! is_last )); then
    entry+=','
  fi

  output+=("     $entry")
done

output+=('  }')
output+=('}')

for line in "${output[@]}"; do
  echo "$line"
done
