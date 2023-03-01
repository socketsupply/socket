#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

declare IPHONEOS_VERSION_MIN="${IPHONEOS_VERSION_MIN:-14.0}"
declare IOS_SIMULATOR_VERSION_MIN="${IOS_SIMULATOR_VERSION_MIN:-$IPHONEOS_VERSION_MIN}"

declare cflags=()
declare arch="$(uname -m)"
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

if [ -z "$ANDROID_HOME" ]; then
  if [[ "$host" = "Darwin" ]]; then
    ANDROID_HOME="$HOME/Library/Android/sdk"
  elif [[ "$host" = "Linux" ]]; then
    ANDROID_HOME="$HOME/android"
  fi
fi

if [[ "$(basename "$CXX")" =~ clang ]]; then
  if [[ "$host" = "Linux" ]] || [[ "$host" = "Win32" ]]; then
    cflags+=("-Wno-unused-command-line-argument")
    cflags+=("-stdlib=libstdc++")
  fi
fi

cflags+=(
  $CFLAG
  $CXXFLAGS
  -std=c++2a
  -I"$root/include"
  -I"$root/build/uv/include"
  -I"$root/build/include"
  -DSSC_BUILD_TIME="$(date '+%s')"
  -DSSC_VERSION_HASH=$(git rev-parse --short=8 HEAD)
  -DSSC_VERSION=$(cat "$root/VERSION.txt")
)

if (( TARGET_OS_IPHONE )) || (( TARGET_IPHONE_SIMULATOR )); then
  if (( TARGET_OS_IPHONE )); then
    ios_sdk_path="$(xcrun -sdk iphoneos -show-sdk-path)"
    cflags+=("-arch arm64")
    cflags+=("-target arm64-apple-ios")
    cflags+=("-Wno-unguarded-availability-new")
    cflags+=("-miphoneos-version-min=$IPHONEOS_VERSION_MIN")
  elif (( TARGET_IPHONE_SIMULATOR )); then
    ios_sdk_path="$(xcrun -sdk iphonesimulator -show-sdk-path)"
    cflags+=("-arch x86_64")
    cflags+=("-mios-simulator-version-min=$IPHONEOS_VERSION_MIN")
  fi

  cflags+=("-iframeworkwithsysroot /System/Library/Frameworks")
  cflags+=("-isysroot $ios_sdk_path/")
  cflags+=("-F $ios_sdk_path/System/Library/Frameworks/")
  cflags+=("-fembed-bitcode")
elif (( TARGET_OS_ANDROID )) || (( TARGET_ANDROID_EMULATOR )); then
  if (( TARGET_OS_ANDROID )); then
    cflags+=("-target aarch64-linux-android")
  elif (( TARGET_ANDROID_EMULATOR )); then
    cflags+=("-target x86_64-linux-android")
  fi

  if [[ "$host" = "Darwin" ]]; then
    cflags+=("--sysroot=$ANDROID_HOME/ndk-bundle/toolchains/llvm/prebuilt/darwin-x86_64/sysroot")
  elif [[ "$host" = "Linux" ]]; then
    cflags+=("--sysroot=$ANDROID_HOME/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/sysroot")
  fi
fi

if (( !TARGET_OS_ANDROID && !TARGET_ANDROID_EMULATOR )); then
  if [[ "$host" = "Darwin" ]]; then
    cflags+=("-ObjC++")
  elif [[ "$host" = "Linux" ]]; then
    cflags+=($(pkg-config --cflags --static gtk+-3.0 webkit2gtk-4.1))
  elif [[ "$host" = "Win32" ]]; then
    cflags+=(
      -D_MT
      -D_DLL
      -DWIN32
      -DWIN32_LEAN_AND_MEAN
      -Xlinker /NODEFAULTLIB:libcmt
      -Wno-nonportable-include-path
    )
  fi
fi

while (( $# > 0 )); do
  cflags+=("$1")
  shift
done

if [[ ! -z "$DEBUG" ]]; then
  cflags+=("-g")
  cflags+=("-O0")
else
  cflags+=("-Os")
fi

echo "${cflags[@]}"
