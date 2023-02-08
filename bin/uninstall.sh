#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

declare PREFIX="${PREFIX:-"/usr/local"}"
declare SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"

declare files=($(find "$SOCKET_HOME"/{bin,include,lib,objects,src,uv} -type f 2>/dev/null))
declare bins=("$PREFIX/bin/ssc")

declare uninstalled=0

for file in "${files[@]}"; do
  if test -f "$file"; then
    echo "# removing $file"
    rm -f "$file"
    (( uninstalled++ ))
  fi
done

for bin in "${bins[@]}"; do
  if test -f "$bin"; then
    echo "# removing $bin"
    rm -f "$bin"
    (( uninstalled++ ))
  fi
done

if (( uninstalled > 0 )); then
  echo "ok - uninstalled $uninstalled files"
else
  exit 1
fi
