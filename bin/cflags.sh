#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

declare IPHONEOS_VERSION_MIN="${IPHONEOS_VERSION_MIN:-15.0}"
declare IOS_SIMULATOR_VERSION_MIN="${IOS_SIMULATOR_VERSION_MIN:-$IPHONEOS_VERSION_MIN}"

declare cflags=()
declare arch="$(uname -m | sed 's/aarch64/arm64/g')"
arch=${ARCH:-$arch}
declare host="${TARGET_HOST:-"$(uname -s)"}"
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

if (( !TARGET_OS_ANDROID && !TARGET_ANDROID_EMULATOR )); then
  if [[ "$(basename "$CXX")" =~ clang ]]; then
    if [[ "$host" = "Linux" ]] || [[ "$host" = "Win32" ]]; then
      cflags+=("-Wno-unused-command-line-argument")
      cflags+=("-stdlib=libstdc++")
    fi
  fi
else
  source "$root/bin/android-functions.sh"
  android_fte > /dev/null
  cflags+=("-DANDROID -pthreads -fexceptions -fPIC -frtti -fsigned-char -D_FILE_OFFSET_BITS=64 -Wno-invalid-command-line-argument -Wno-unused-command-line-argument")
  cflags+=("-I$(dirname $NDK_BUILD)/sources/cxx-stl/llvm-libc++/include")
  cflags+=("-I$(dirname $NDK_BUILD)/sources/cxx-stl/llvm-libc++abi/include")
fi

cflags+=(
  $CFLAGS
  $CXXFLAGS
  -std=c++2a
  -ferror-limit=6
  -I"$root/include"
  -I"$root/build/uv/include"
  -I"$root/build"
  -I"$root/build/llama"
  -I"$root/build/llama/common"
  -I"$root/build/include"
  -DSOCKET_RUNTIME_BUILD_TIME="$(date '+%s')"
  -DSOCKET_RUNTIME_VERSION_HASH=$(git rev-parse --short=8 HEAD)
  -DSOCKET_RUNTIME_VERSION=$(cat "$root/VERSION.txt")
)

if (( TARGET_OS_IPHONE )) || (( TARGET_IPHONE_SIMULATOR )); then
  if (( TARGET_OS_IPHONE )); then
    ios_sdk_path="$(xcrun -sdk iphoneos -show-sdk-path)"
    cflags+=("-arch arm64")
    cflags+=("-target arm64-apple-ios")
    cflags+=("-Wno-unguarded-availability-new")
    if [ -n "$IPHONEOS_VERSION_MIN" ]; then
      cflags+=("-miphoneos-version-min=$IPHONEOS_VERSION_MIN")
    fi
  elif (( TARGET_IPHONE_SIMULATOR )); then
    ios_sdk_path="$(xcrun -sdk iphonesimulator -show-sdk-path)"
    cflags+=("-arch $arch")
    if [ -n "$IOS_SIMULATOR_VERSION_MIN" ]; then
      cflags+=("-mios-simulator-version-min=$IOS_SIMULATOR_VERSION_MIN")
    fi
  fi

  cflags+=("-iframeworkwithsysroot /System/Library/Frameworks")
  cflags+=("-isysroot $ios_sdk_path/")
  cflags+=("-F $ios_sdk_path/System/Library/Frameworks/")
  cflags+=("-fembed-bitcode")
fi

if (( !TARGET_OS_ANDROID && !TARGET_ANDROID_EMULATOR )); then
  if [[ "$host" = "Darwin" ]]; then
    cflags+=("-ObjC++")
    cflags+=("-fPIC")
  elif [[ "$host" = "Linux" ]]; then
    cflags+=($(pkg-config --cflags --static gtk+-3.0 webkit2gtk-4.1) -fPIC)
  elif [[ "$host" = "Win32" ]]; then
    # https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-170
    # Because we can't pass /MT[d] directly, we have to manually set the flags
    cflags+=(
      -D_MT
      -D_DLL
      -DWIN32
      -DWIN32_LEAN_AND_MEAN
      "-Xlinker /NODEFAULTLIB:libcmt"
      -Wno-nonportable-include-path
    )
    if [[ -n "$DEBUG" ]]; then
      cflags+=("-D_DEBUG")
    fi

    ## TODO(@jwerle): figure this out for macOS
    if [[ "$(uname -s)" == "Linux" ]]; then
      cflags+=(
        "-fdeclspec"
        "-I/usr/share/mingw-w64/include"
        "-I/usr/include/x86_64-linux-gnu/c++/12/"
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/"
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-posix/include/c++"
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++"
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++/x86_64-w64-mingw32"
        "-I/usr/lib/gcc/x86_64-w64-mingw32/10-win32/include/c++/backward"
        "-DWIN32"
        "-DWINVER=0x0A00"
        "-D_WIN32_WINNT=0x0A00"
        "-D_WIN32"
        "-D_WIN64"
        "-D_MSC_VER=1940"
        "-D_MSC_FULL_VER=193933519"
        "-D_MSC_BUILD=0"
        "-D_GLIBCXX_HAS_GTHREADS=1"
        "-DSOCKET_RUNTIME_CROSS_COMPILED_HOST=1"
        "-DSOCKET_RUNTIME_PLATFORM_WANTS_MINGW=1"
        $(pkg-config gthread-2.0 --cflags)
      )
    fi
  fi
fi

while (( $# > 0 )); do
  cflags+=("$1")
  shift
done

if [[ -n "$DEBUG" ]]; then
  cflags+=("-g")
  cflags+=("-O0")
  cflags+=("-DSOCKET_RUNTIME_BUILD_DEBUG=1")
else
  cflags+=("-Os")
fi

echo "${cflags[@]}"
