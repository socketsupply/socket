#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

declare ldflags=()

declare args=()
declare arch="$(uname -m | sed 's/aarch64/arm64/g')"
declare host="$(uname -s)"
declare platform="desktop"

declare ios_sdk_path=""

if [[ "$host" = "Linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    host="Win32"
  fi
fi

if [[ "$host" == *"MINGW64_NT"* ]]; then
  host="Win32"
fi

declare d=""
if [[ "$host" == "Win32" ]]; then
  # We have to differentiate release and debug for Win32
  if [[ -n "$DEBUG" ]]; then
    d="d"
  fi
fi

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

if [[ "$host" = "Darwin" ]]; then
  if (( !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR )); then
    ldflags+=("-framework" "Cocoa")
    ldflags+=("-framework" "Carbon")
  fi

  if (( TARGET_OS_IPHONE )) || (( TARGET_IPHONE_SIMULATOR )); then
    ldflags+=("-framework" "UIKit")

    if (( TARGET_OS_IPHONE )); then
      ios_sdk_path="$(xcrun -sdk iphoneos -show-sdk-path)"
      ldflags+=("-arch arm64")
    elif (( TARGET_IPHONE_SIMULATOR )); then
      ios_sdk_path="$(xcrun -sdk iphonesimulator -show-sdk-path)"
      ldflags+=("-arch x86_64")
    fi

    ldflags+=("-isysroot $ios_sdk_path/")
    ldflags+=("-iframeworkwithsysroot /System/Library/Frameworks/")
    ldflags+=("-F $ios_sdk_path/System/Library/Frameworks/")
  fi

  ldflags+=("-framework" "CoreFoundation")
  ldflags+=("-framework" "CoreBluetooth")
  ldflags+=("-framework" "CoreLocation")
  ldflags+=("-framework" "Foundation")
  ldflags+=("-framework" "Network")
  ldflags+=("-framework" "UniformTypeIdentifiers")
  ldflags+=("-framework" "WebKit")
  ldflags+=("-framework" "Metal")
  ldflags+=("-framework" "Accelerate")
  ldflags+=("-framework" "UserNotifications")
  ldflags+=("-framework" "OSLog")
  ldflags+=("-ldl")
  ldflags+=("-lggml")
  ldflags+=("-lggml-cpu")
  ldflags+=("-lggml-base")
  ldflags+=("-lggml-blas")
  ldflags+=("-lggml-metal")
elif [[ "$host" = "Linux" ]]; then
  ldflags+=("-ldl")
  ldflags+=($(pkg-config --libs dbus-1 gtk+-3.0 webkit2gtk-4.1))
elif [[ "$host" = "Win32" ]]; then
  if [[ -n "$DEBUG" ]]; then
    # https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-170
    # TODO(@heapwolf): Populate from vcvars64.bat
    IFS=',' read -r -a libs <<< "$WIN_DEBUG_LIBS"
    for (( i = 0; i < ${#libs[@]}; ++i )); do
      ldflags+=("${libs[$i]}")
    done
  fi
fi

ldflags+=("-L$root/build/$arch-$platform/lib$d")

for (( i = 0; i < ${#args[@]}; ++i )); do
  ldflags+=("${args[$i]}")
done

echo "${ldflags[@]}"
