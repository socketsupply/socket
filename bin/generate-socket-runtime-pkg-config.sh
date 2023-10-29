#!/usr/bin/env bash
# vim: set syntax=bash:

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/mush.sh"
source "$root/bin/functions.sh"
source "$root/bin/android-functions.sh"

declare platform="desktop"
declare host="$(host_os)"
declare arch="$(host_arch)"

if (( TARGET_OS_IPHONE )); then
  arch="arm64"
  platform="iPhoneOS"
elif (( TARGET_IPHONE_SIMULATOR )); then
  arch="x86_64"
  platform="iPhoneSimulator"
elif (( TARGET_OS_ANDROID )); then
  arch="aarch64"
  platform="android"
elif (( TARGET_ANDROID_EMULATOR )); then
  arch="x86_64"
  platform="android"
fi

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
    pass_force="$arg"
    force=1; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    if [[ "$1" = "ios" ]] || [[ "$1" = "iPhoneOS" ]] || [[ "$1" = "iphoneos" ]]; then
      arch="arm64"
      platform="iPhoneOS";
      export TARGET_OS_IPHONE=1
    elif [[ "$1" = "ios-simulator" ]] || [[ "$1" = "iPhoneSimulator" ]] || [[ "$1" = "iphonesimulator" ]]; then
      [[ -z "$arch" ]] && arch="x86_64"
      platform="iPhoneSimulator";
      export TARGET_IPHONE_SIMULATOR=1
    elif [[ "$1" = "android" ]] || [[ "$1" = "android-emulator" ]]; then
      platform="android";
      export TARGET_OS_ANDROID=1
    else
      platform="$1";
    fi
    shift
    continue
  fi

  # Don't rebuild if header mtimes are newer than .o files - Be sure to manually delete affected assets as required
  if [[ "$arg" == "--ignore-header-mtimes" ]]; then
    ignore_header_mtimes=1; continue
  fi

  args+=("$arg")
done

declare input="$root/socket-runtime.pc.in"
declare output="$root/build/$arch-$platform/pkgconfig/socket-runtime.pc"

mkdir -p "$(dirname "$output")"

declare version="$(cat "$root/VERSION.txt")"
declare lib_directory="$root/build/$arch-$platform/lib"
declare include_directory="$root/build/$arch-$platform/include"

declare ldflags=()
declare dependencies=()
declare cflags=(
  "-0s"
  "-std=c++2a"
  "-fvisibility=hidden"
)

if [ "$platform" == "iPhoneOS" ]; then
  platform="ios"
elif [ "$platform" == "iPhoneSimulator" ]; then
  platform="ios-simulator"
fi

if [ "$host" == "Linux" ]; then
  if [ "$platform" == "desktop" ]; then
    if [[ "$(basename "$CXX")" =~ clang ]]; then
      cflags+=("-stdlib=libstdc++")
      cflags+=("-Wno-unused-command-line-argument")
    fi
    cflags+=("-fPIC")
    ldflags+=("-ldl")
    dependencies+=("gtk+-3.0" "webkit2gtk-4.1")
  fi
elif [ "$host" == "Win32" ]; then
  if [ "$platform" == "desktop" ]; then
    if [[ "$(basename "$CXX")" =~ clang ]]; then
      cflags+=("-Wno-unused-command-line-argument")
      cflags+=("-stdlib=libstdc++")
    fi

    # https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-library-features?view=msvc-170
    # Because we can't pass /MT[d] directly, we have to manually set the flags
    cflags+=(
      "-D_MT"
      "-D_DLL"
      "-DWIN32"
      "-DWIN32_LEAN_AND_MEAN"
      "-Xlinker" "/NODEFAULTLIB:libcmt"
      "-Wno-nonportable-include-path"
    )
  fi
elif [ "$host" == "Darwin" ]; then
  if [ "$platform" == "desktop" ]; then
    cflags+=("-ObjC++")
    cflags+=("-fPIC")
  fi
fi

if [ "$platform" == "android" ]; then
  android_fte > /dev/null 2>&1
  cflags+=("-DANDROID -pthreads -fexceptions -fPIC -frtti -fsigned-char -D_FILE_OFFSET_BITS=64 -Wno-invalid-command-line-argument -Wno-unused-command-line-argument")
  cflags+=("-I$(dirname $NDK_BUILD)/sources/cxx-stl/llvm-libc++/include")
  cflags+=("-I$(dirname $NDK_BUILD)/sources/cxx-stl/llvm-libc++abi/include")
fi

if [ "$platform" == "ios" ] || [ "$platform" == "ios-simulator" ]; then
  if [ "$host" != "Darwin" ]; then
    echo "error: Cannot generate pkgconfig file for iPhoneOS or iPhoneSimulator on '$host'" >&2
    exit 1
  fi

  if [ "$platform" == "ios" ]; then
    ios_sdk_path="$(xcrun -sdk iphoneos -show-sdk-path)"
    cflags+=("-arch arm64")
    cflags+=("-target arm64-apple-ios")
    cflags+=("-Wno-unguarded-availability-new")
    cflags+=("-miphoneos-version-min=$IPHONEOS_VERSION_MIN")
  elif [ "$platform" == "ios-simulator" ]; then
    ios_sdk_path="$(xcrun -sdk iphonesimulator -show-sdk-path)"
    cflags+=("-arch $arch")
    cflags+=("-mios-simulator-version-min=$IPHONEOS_VERSION_MIN")
  fi
  cflags+=("-iframeworkwithsysroot /System/Library/Frameworks")
  cflags+=("-isysroot $ios_sdk_path/")
  cflags+=("-F $ios_sdk_path/System/Library/Frameworks/")
  cflags+=("-fembed-bitcode")
fi

export CFLAGS="${cflags[@]}"
export LDFLAGS="${ldflags[@]}"
export DEPENDENCIES="${dependencies[@]}"
export VERSION="$version"
export LIB_DIRECTORY="$lib_directory"
export INCLUDE_DIRECTORY="$include_directory"

rm -f "$output"

if ! cat "$input" | mush > "$output"; then
  exit $?
fi

echo "Wrote pkgconfig file to '$output'"
