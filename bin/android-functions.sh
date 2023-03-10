#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"

function android_host_platform() {
  local platform=$1
  case $platform in
    Darwin|darwin)
      echo -n "darwin"
      ;;
    Linux|linux)
      echo -n "linux"
      ;;
    Win32)
      echo -n "windows"
      ;;
    *)
      echo "Unknown-host-platform-platform-$platform"
      exit 1
      ;;
  esac
}

function android_eabi() {
  local arch=$1
  case $arch in
    armeabi-v7a)
      echo -n "eabi"
      ;;
    *)
      echo -n ""
    ;;
  esac
}

function android_arch() {
  local arch=$1
  case $arch in
    arm64-v8a)
      echo -n "aarch64"
      ;;
    armeabi-v7a)
      echo -n "armv7a"
      ;;
    x86)
      echo -n "i686"
      ;;
    x86-64|x86_64)
      echo -n "x86_64"
      ;;
    *)
      echo "Unknown-android-arch-$arch"
      exit 1
      ;;
  esac
}

function android_machine_arch() {
  local arch=$1
  case $arch in
    arm64-v8a)
      echo -n ""
      ;;
    armeabi-v7a)
      echo -n "-m=armelf_linux_eabi"
      ;;
    x86)
      echo -n "-m=elf_i386"
      ;;
    x86-64|x86_64)
      echo -n ""
      ;;
    *)
      echo "Unknown-android-arch-$arch"
      exit 1
      ;;
  esac
}

function android-arch-includes() {
  arch=$1
  include=(
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/$(android_arch "$arch")"-linux-android"$(android_eabi $arch)"
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include"
    "--sysroot=$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/$(android_arch "$arch")"-linux-android"$(android_eabi $arch)"
  )

  echo "${include[@]}"
}

if [[ -z "$ANDROID_HOME" ]]; then
  echo "ANDROID_HOME not set."
  DEPS_ERROR=1
else
  if [[ "$host" == "Win32" ]]; then
    ANDROID_HOME=$(cygpath -u $ANDROID_HOME)
  fi
fi

# if [[ -z $JAVA_HOME ]]; then
#   echo "JAVA_HOME not set."
#   DEPS_ERROR=1
# fi

declare cmd=""
declare exe=""
if [[ "$host" == "Win32" ]]; then
  cmd=".cmd"
  exe=".exe"
fi

declare NDK_VERSION="25.0.8775105"
declare NDK_PLATFORM="33"
NDK_BUILD="$ANDROID_HOME/ndk/$NDK_VERSION/ndk-build$cmd"
if ! test -f $NDK_BUILD; then
  echo "ndk-build not at $NDK_BUILD"
  DEPS_ERROR=1
fi

if [[ ! -z "$ANDROID_HOME" ]]; then
  if [[ ! -n $CI ]] || [[ -n $SSC_ANDROID_CI ]]; then
    BUILD_ANDROID=1
  fi
fi
