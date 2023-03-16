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
      echo -n $arch
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
    x86-64)
      echo -n "x86_64"
      ;;
    *)
      echo -n $arch
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
      echo -n $arch
      ;;
  esac
}

function android_clang() {
  # get abi specific clang call for host, host arch and target arch (target host=linux)
  ANDROID_HOME=$1
  NDK_VERSION=$2
  host=$3
  host_arch=$4
  arch=$5
  plusplus=$6
  echo "$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/"$(android_host_platform $host)"-"$(android_arch "$host_arch")"/bin/clang$plusplus --target="$(android_arch "$arch")"-linux-android"$(android_eabi $arch)
}

function android_ar() {
  # Note that this one is not target arch specific
  ANDROID_HOME=$1
  NDK_VERSION=$2
  host=$3
  host_arch=$4
  echo "$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/"$(android_host_platform $host)"-"$(android_arch "$host_arch")"/bin/llvm-ar"
}

function android_arch_includes() {
  #get abi specific includes and sysroot
  arch=$1
  include=(
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/$(android_arch "$arch")"-linux-android"$(android_eabi $arch)"
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include"
    "--sysroot=$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/$(android_arch "$arch")"-linux-android"$(android_eabi $arch)"
  )

  echo "${include[@]}"
}

function android_supported_abis() {
  abis=(
    "arm64-v8a"
    "armeabi-v7a"
    "x86"
    "x86_64"
  )

  echo "${abis[@]}"
}

if [[ -z "$ANDROID_HOME" ]]; then
  echo "ANDROID_HOME not set."
  DEPS_ERROR=1
else
  if [[ "$host" == "Win32" ]]; then
    ANDROID_HOME=$(cygpath -u $ANDROID_HOME)
  fi
fi

declare cmd=""
declare exe=""
if [[ "$host" == "Win32" ]]; then
  cmd=".cmd"
fi

declare NDK_VERSION="25.0.8775105"
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
