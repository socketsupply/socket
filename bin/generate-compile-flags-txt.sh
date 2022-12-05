#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare arch="$(uname -m)"
declare platform="desktop"
declare force=0
declare args=()

if (( TARGET_OS_IPHONE )); then
  arch="arm64"
  platform="iPhoneOS"
elif (( TARGET_IPHONE_SIMULATOR )); then
  arch="x86_64"
  platform="iPhoneSimulator"
elif (( TARGET_OS_ANDROID )); then
  arch="aarch64"
  platform="Android"
elif (( TARGET_ANDROID_EMULATOR )); then
  arch="x86_64"
  platform="AndroidEmulator"
fi

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
   force=1; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    if [[ "$1" = "ios" ]] || [[ "$1" = "iPhoneOS" ]] || [[ "$1" = "iphoneos" ]]; then
      arch="arm64"
      platform="iPhoneOS";
      export TARGET_OS_IPHONE=1
    elif [[ "$1" = "ios-simulator" ]] || [[ "$1" = "iPhoneSimulator" ]] || [[ "$1" = "iphonesimulator" ]]; then
      arch="x86_64"
      platform="iPhoneSimulator";
      export TARGET_IPHONE_SIMULATOR=1
    elif [[ "$1" = "android" ]] || [[ "$1" = "Android" ]]; then
      arch="aarch64"
      platform="Android";
      export TARGET_OS_ANDROID=1
    elif [[ "$1" = "android-emulator" ]] || [[ "$1" = "AndroidEmulator" ]]; then
      arch="x86_64"
      platform="AndroidEmulator";
      export TARGET_ANDROID_EMULATOR=1
    else
      platform="$1";
    fi
    shift
    continue
  fi

  args+=("$arg")
done

function generate () {
  local cflags=($("$root/bin/cflags.sh" "${args[@]}") -ferror-limit=0)

  for flag in "${cflags[@]}"; do
    echo "$flag"
  done
}

generate > "$root/compile_flags.txt"
