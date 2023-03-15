#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"
echo $root

declare DARWIN_DEFAULT_ANDROID_HOME=$HOME/Library/Android/sdk
declare WIN32_DEFAULT_ANDROID_HOME=$LOCALAPPDATA\\Android\\Sdk
declare LINUX_DEFAULT_ANDROID_HOME=$HOME/Android/Sdk

declare ANDROID_SDK_MANAGER_SEARCH_PATHS=(
  "cmdline-tools/latest/bin"
  "cmdline-tools/bin"
  "tools/bin"
)

# TODO(mribbons): ubuntu / apt libs: apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386 lib32z1 libbz2-1.0:i386
# TODO(mribbons): gradle

function get_android_paths() {
  host=$(host_os)
  ANDROID_HOME_SEARCH_PATHS=()
  if [ -n "$ANDROID_HOME" ]; then
    ANDROID_HOME_SEARCH_PATHS+=("$ANDROID_HOME")
  fi

  JAVA_HOME_SEARCH_PATHS=()
  if [ -n "$JAVA_HOME" ]; then
    JAVA_HOME_SEARCH_PATHS+=("$JAVA_HOME")
  fi

  GRADLE_SEARCH_PATHS=()

  if [ -n "$GRADLE_HOME" ]; then
    GRADLE_SEARCH_PATHS+=("$GRADLE_HOME")
  fi
  
  if [ -n "$HOME" ]; then
    GRADLE_SEARCH_PATHS+=("$HOME/.gradle")
  fi

  # TODO(mribbons): SDK manager cmd line tools not installed by default
  if [ -z "$ANDROID_HOME" ]; then
    # Only attempt default homes if $ANDROID_HOME not defined
    if [[ "$host" = "Darwin"  ]]; then
      ANDROID_HOME_SEARCH_PATHS+=("$DARWIN_DEFAULT_ANDROID_HOME")
      JAVA_HOME_SEARCH_PATHS+=("$HOME/homebrew")
      JAVA_HOME_SEARCH_PATHS+=("$HOME/.vscode")
      JAVA_HOME_SEARCH_PATHS+=("$HOME/Applications")
    elif [[ "$host" = "Linux"  ]]; then
      ANDROID_HOME_SEARCH_PATHS+=("$LINUX_DEFAULT_ANDROID_HOME")
      ANDROID_HOME_SEARCH_PATHS+=("$HOME")
      # Java should be unpacked alongside Android Studio
      JAVA_HOME_SEARCH_PATHS+=("$HOME")
      JAVA_HOME_SEARCH_PATHS+=("/opt")
    elif [[ "$host" = "Win32"  ]]; then
      ANDROID_HOME_SEARCH_PATHS+=("$WIN32_DEFAULT_ANDROID_HOME")
      JAVA_HOME_SEARCH_PATHS+=("$PROGRAMFILES/")
    fi
  fi

  local _ah
  local _sdk
  local _jh
  local _gh

  for android_home_test in "${ANDROID_HOME_SEARCH_PATHS[@]}"; do
    for sdk_man_test in "${ANDROID_SDK_MANAGER_SEARCH_PATHS[@]}"; do
      # r=$(test_android_path "$android_home_test" "$sdk_man_test")
      if [[ -n $VERBOSE ]]; then
        echo "Checking $android_home_test/$sdk_man_test/sdkmanager$bat"
      fi
      if [[ -f "$android_home_test/$sdk_man_test/sdkmanager$bat" ]]; then
        # Can't break out of for in;
        _ah="$android_home_test"
        _sdk="$sdk_man_test/sdkmanager$bat"
        break
      fi
    done
  done

  temp=$(mktemp)

  for java_home_test in "${JAVA_HOME_SEARCH_PATHS[@]}"; do
    if [[ -n $VERBOSE ]]; then
      echo "find $java_home_test -type f -name 'javac$exe' -print0 2>/dev/null | while IFS= read -r -d '' javac$exe"
    fi
    find "$java_home_test" -type f -name "javac$exe" -print0 2>/dev/null | while IFS= read -r -d '' javac
    do
      # subshell, output to file
      echo "$(dirname "$(dirname "$javac")")" > "$temp"
      break
    done
  done

  if [ -f "$temp" ]; then
    _jh=$(cat "$temp")
    rm "$temp"
  fi

  for gradle_test in "${GRADLE_SEARCH_PATHS[@]}"; do
    if [[ -n $VERBOSE ]]; then
      echo "find $gradle_test -type f -name 'gradle' -print0 2>/dev/null | while IFS= read -r -d '' gradle"
    fi
    find "$gradle_test" -type f -name "gradle$bat" -print0 2>/dev/null | while IFS= read -r -d '' gradle
    do
      # subshell, output to file
      echo "$(dirname "$(dirname "$gradle")")" > "$temp"
      break
    done
  done

  if [ -f "$temp" ]; then
    _gh=$(cat "$temp")
    rm "$temp"
  fi

  if [[ -n "$_ah" ]]; then 
    ANDROID_HOME="$_ah"
    export ANDROID_HOME
  fi
  
  if [[ -n "$_sdk" ]]; then 
    ANDROID_SDK_MANAGER="$_sdk"
    export ANDROID_SDK_MANAGER
  fi
  
  if [[ -n "$_jh" ]]; then 
    JAVA_HOME="$_jh"
    export JAVA_HOME
  fi
  
  if [[ -n "$_gh" ]]; then 
    GRADLE_HOME="$_gh"
    export GRADLE_HOME
  fi
}

function android_host_platform() {
  local platform=$1
  case "$platform" in
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
      echo -n "$arch"
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
    aarch64|arm64|x86-64)
      echo -n "x86_64"
      ;;
    *)
      echo -n "$arch"
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
      echo -n "$arch"
      ;;
  esac
}

function android_clang() {
  # get abi specific clang call for host, host arch and target arch (target host=linux)
  local ANDROID_HOME=$1
  local NDK_VERSION=$2
  local host=$3
  local host_arch=$4
  local arch=$5
  local plusplus=$6
  echo "$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/$(android_host_platform "$host")-$(android_arch "$host_arch")/bin/clang$plusplus --target=$(android_arch "$arch")-linux-android$(android_eabi "$arch")"
}

function android_ar() {
  # Note that this one is not target arch specific
  local ANDROID_HOME=$1
  local NDK_VERSION=$2
  local host=$3
  local host_arch=$4
  echo "$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/$(android_host_platform "$host")-$(android_arch "$host_arch")/bin/llvm-ar"
}

function android_arch_includes() {
  #get abi specific includes and sysroot
  local arch=$1
  local include=(
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/$(android_arch "$arch")-linux-android$(android_eabi "$arch")"
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include"
    "--sysroot=$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/$(android_arch "$arch")-linux-android$(android_eabi "$arch")"
  )

  echo "${include[@]}"
}

function android_min_expected_static_lib_size() {
  local lib=$1
  case $lib in
    libsocket-runtime)
      echo -n "1048576"
      ;;
    libuv)
      echo -n "800000"
      ;;
    *)
      echo "Unsupported lib: $lib"
      exit 1
  esac
}

function android_supported_abis() {
  if [[ -n "$ANDROID_SUPPORTED_ABIS" ]]; then
    echo "${ANDROID_SUPPORTED_ABIS[@]}"
  else
    local abis=(
      "arm64-v8a"
      "armeabi-v7a"
      "x86"
      "x86_64"
    )

    echo "${abis[@]}"
  fi
}
export DEPS_ERROR


if [[ -z "$ANDROID_HOME" ]]; then
  echo "Android dependencies: ANDROID_HOME not set."
  DEPS_ERROR=1
else
  if [[ "$host" == "Win32" ]]; then
    ANDROID_HOME=$(cygpath -u "$ANDROID_HOME")
  fi
fi

declare cmd=""
declare exe=""
declare bat=""
if [[ "$host" == "Win32" ]]; then
  exe=".exe"
  cmd=".cmd"
  bat=".bat"
fi

declare NDK_VERSION="25.0.8775105"
if [[ -n "$host" ]]; then
  export NDK_BUILD="$ANDROID_HOME/ndk/$NDK_VERSION/ndk-build$cmd"
  if ! test -f "$NDK_BUILD"; then
    echo "Android dependencies: ndk-build not at $NDK_BUILD"
    DEPS_ERROR=1
  fi
fi

export BUILD_ANDROID

if [[ -n "$ANDROID_HOME" ]]; then
  if [[ -z $CI ]] || [[ -n $SSC_ANDROID_CI ]]; then
    BUILD_ANDROID=1
  fi
fi
