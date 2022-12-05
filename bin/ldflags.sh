#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare ldflags=()

declare args=()
declare arch="$(uname -m)"
declare platform="desktop"
declare ios_sdk_path=""

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

if [[ "$(uname -s)" = "Darwin" ]]; then
  if (( !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR )); then
    ldflags+=("-framework" "Cocoa")
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
  ldflags+=("-framework" "Foundation")
  ldflags+=("-framework" "Network")
  ldflags+=("-framework" "UniformTypeIdentifiers")
  ldflags+=("-framework" "WebKit")
  ldflags+=("-framework" "UserNotifications")
elif [[ "$(uname -s)" = "Linux" ]]; then
  ldflags+=($(pkg-config --libs gtk+-3.0 webkit2gtk-4.1))
fi

ldflags+=("-L$root/build/$arch-$platform/lib")

for (( i = 0; i < ${#args[@]}; ++i )); do
  ldflags+=("${args[$i]}")
done

echo "${ldflags[@]}"
