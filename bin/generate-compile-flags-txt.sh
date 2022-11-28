#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare arch="$(uname -m)"
declare platform="desktop"
declare force=0

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    platform="$1"; shift; continue
  fi

  args+=("$arg")
done

function generate () {
  local cflags=($("$root/bin/cflags.sh") -ferror-limit=0)

  for flag in "${cflags[@]}"; do
    echo "$flag"
  done
}

generate > "$root/compile_flags.txt"
