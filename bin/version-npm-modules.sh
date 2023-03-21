#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare archs=()
declare platform="$(uname | tr '[[:upper:]]' '[[:lower:]]')"

if [ "$(uname -m)" = "x86_64" ]; then
  archs+=("x64")
elif [ "$(uname -m)" = "aarch64" ]; then
  archs+=("arm64")
else
  archs+=("$(uname -m)")
fi

if [[ "$platform" = "linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    platform="win32"
  fi
fi

for arch in "${archs[@]}"; do
  declare package="@socketsupply/socket-$platform-$arch"
  cd "$root/npm/packages/$package" || exit $?
  echo "# $package"
  npm version "$@" || exit $?
done
