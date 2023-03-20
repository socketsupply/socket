#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"

declare ANDROID_SETTINGS_FILENAME=".android-rc"

declare -A DEFAULT_ANDROID_HOME
DEFAULT_ANDROID_HOME[Darwin]=$HOME/Library/Android/sdk 
DEFAULT_ANDROID_HOME[Win32]=$LOCALAPPDATA\\Android\\Sdk
DEFAULT_ANDROID_HOME[Linux]=$HOME/Android/Sdk

declare ANDROID_SDK_MANAGER_SEARCH_PATHS=(
  "cmdline-tools/latest/bin"
  "cmdline-tools/bin"
  "tools/bin"
)

declare ANDROID_PLATFORM_TOOLS_URI_TEMPLATE="https://dl.google.com/android/repository/platform-tools-latest-{os}.zip"
declare ANDROID_PLATFORM_TOOLS_PAGE_URI="https://developer.android.com/studio/releases/platform-tools"
declare ANDROID_COMMAND_LINE_TOOLS_URI_TEMPLATE="https://dl.google.com/android/repository/commandlinetools-{os}-9477386_latest.zip"
declare ANDROID_STUDIO_PAGE_URI="https://developer.android.com/studio"
declare JDK_URI_TEMPLATE="https://download.java.net/java/GA/jdk19.0.2/fdb695a9d9064ad6b064dc6df578380c/7/GPL/openjdk-19.0.2_{os}-{arch}_bin.{format}"
declare GRADLE_URI_TEMPLATE="https://services.gradle.org/distributions/gradle-8.0.2-bin.zip"

# TODO(mribbons): ubuntu / apt libs: apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386 lib32z1 libbz2-1.0:i386

function get_android_default_search_paths() {
  host="$(host_os)"
  ANDROID_HOME_SEARCH_PATHS=()
  if [ -n "$ANDROID_HOME" ]; then
    ANDROID_HOME_SEARCH_PATHS+=("$ANDROID_HOME")
  fi

  ANDROID_HOME_SEARCH_PATHS+=("${DEFAULT_ANDROID_HOME["$host"]}")

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
      JAVA_HOME_SEARCH_PATHS+=("$HOME/homebrew")
      JAVA_HOME_SEARCH_PATHS+=("$HOME/.vscode")
      JAVA_HOME_SEARCH_PATHS+=("$HOME/Applications")
    elif [[ "$host" = "Linux"  ]]; then
      ANDROID_HOME_SEARCH_PATHS+=("$HOME")
      # Java should be unpacked alongside Android Studio
      JAVA_HOME_SEARCH_PATHS+=("$HOME")
      JAVA_HOME_SEARCH_PATHS+=("/opt")
    elif [[ "$host" = "Win32"  ]]; then
      JAVA_HOME_SEARCH_PATHS+=("$PROGRAMFILES/")
    fi
  fi

  export ANDROID_HOME_SEARCH_PATHS
  export JAVA_HOME_SEARCH_PATHS
  export GRADLE_SEARCH_PATHS
}


function get_android_paths() {
  get_android_default_search_paths

  local _ah
  local _sdk
  local _jh
  local _gh

  for android_home_test in "${ANDROID_HOME_SEARCH_PATHS[@]}"; do
    for sdk_man_test in "${ANDROID_SDK_MANAGER_SEARCH_PATHS[@]}"; do
      if [[ -z "$_ah" ]]; then
        if [[ -n $VERBOSE ]]; then
          echo "Checking $android_home_test/$sdk_man_test/sdkmanager$bat"
        fi
        if [[ -f "$android_home_test/$sdk_man_test/sdkmanager$bat" ]]; then
          _ah="$android_home_test"
          _sdk="$sdk_man_test/sdkmanager$bat"
        fi
      fi
    done
  done

  temp=$(mktemp)

  for java_home_test in "${JAVA_HOME_SEARCH_PATHS[@]}"; do
    if [[ -f "$java_home_test/bin/javac$exe" ]]; then
      echo "$java_home_test" > "$temp"
      break
    fi

    if [[ -n $VERBOSE ]]; then
      echo "find $java_home_test -type f -name "javac$exe"' -print0 2>/dev/null | while IFS= read -r -d '' javac"
    fi
    find "$java_home_test" -type f -name "javac$exe" -print0 2>/dev/null | while IFS= read -r -d '' javac
    do
      # break doesn't work here, check that we don't have a result
      if [[ $(stat_size "$temp") == 0 ]]; then
        if [[ -n $VERBOSE ]]; then
          echo "Found $javac"
        fi
        # subshell, output to file
        echo "$(dirname "$(dirname "$javac")")" > "$temp"
      fi
    done
  done

  if [[ $(stat_size "$temp") != 0 ]]; then
    _jh=$(cat "$temp")
  fi

  echo -n > "$temp"

  for gradle_test in "${GRADLE_SEARCH_PATHS[@]}"; do
    if [[ -f "$gradle_test/bin/gradle$bat" ]]; then
      echo "$gradle_test" > "$temp"
      break
    fi

    if [[ -n $VERBOSE ]]; then
      echo "find $gradle_test -type f -name 'gradle' -print0 2>/dev/null | while IFS= read -r -d '' gradle"
    fi
    find "$gradle_test" -type f -name "gradle$bat" -print0 2>/dev/null | while IFS= read -r -d '' gradle
    do
      # break doesn't work here, check that we don't have a result
      if [[ $(stat_size "$temp") == 0 ]]; then
      if [[ -n $VERBOSE ]]; then
        echo "Found $gradle"
      fi
        # subshell, output to file
        echo "$(dirname "$(dirname "$gradle")")" > "$temp"
      fi
    done
  done

  if [[ $(stat_size "$temp") != 0 ]]; then
    _gh=$(cat "$temp")
  fi
  rm "$temp"

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

function load_android_paths() {
  if [[ -f "$ANDROID_SETTINGS_FILENAME" ]]; then
    source "$ANDROID_SETTINGS_FILENAME"
  fi
}

function format_android_paths() {
  echo "export ANDROID_HOME=\"$(escape_path "$ANDROID_HOME")\""
  echo "export JAVA_HOME=\"$(escape_path "$JAVA_HOME")\""
  echo "export ANDROID_SDK_MANAGER=\"$(escape_path "$ANDROID_SDK_MANAGER")\""
  echo "export GRADLE_HOME=\"$(escape_path "$GRADLE_HOME")\""
}

function save_android_paths() {
  # Maintain mtime on $ANDROID_SETTINGS_FILENAME, only update if changed
  temp=$(mktemp)
  format_android_paths > "$temp"
  if [[ ! -f "$ANDROID_SETTINGS_FILENAME" ]]; then
    mv "$temp" "$ANDROID_SETTINGS_FILENAME"
  else
    old_hash=$(sha512sum "$ANDROID_SETTINGS_FILENAME")
    new_hash=$(sha512sum "$temp")

    if [[ "$old_hash" != "$new_hash" ]]; then
      mv "$temp" "$ANDROID_SETTINGS_FILENAME"
    else
      rm "$temp"
    fi
  fi
}


function build_android_platform_tools_uri() {
  os="$host"

  uri=${ANDROID_PLATFORM_TOOLS_URI_TEMPLATE/\{os\}/"$(android_host_platform "$os")"}
  echo "$uri"
}

function build_android_command_line_tools_uri() {
  os="$host"
  os="$(lower "$os")"

  if [[ "$os" == "darwin" ]]; then
    os="mac"
  elif [[ "$os" == "win32" ]]; then
    os="win"
  fi

  uri=${ANDROID_COMMAND_LINE_TOOLS_URI_TEMPLATE/\{os\}/"$os"}
  echo "$uri"
}


function build_jdk_uri() {
  os="$host"
  arch=$(uname -m)
  format="tar.gz"

  if [[ -n "$1" ]]; then
    os="$1"
  fi
  os="$(lower "$os")"
  
  if [[ -n "$2" ]]; then
    arch="$2"
  fi

  if [[ "$os" == "darwin" ]]; then
    os="macos"
  elif [[ "$os" == "win32" ]]; then
    os="windows"
    format="zip"
  fi

  if [[ "$arch" == "arm64" ]]; then
    arch="aarch64"
  elif [[ "$arch" == "x86_64" ]]; then
    arch="x64"
  fi

  uri=${JDK_URI_TEMPLATE/\{os\}/$os}
  uri=${uri/\{arch\}/$arch}
  uri=${uri/\{format\}/$format}
  echo "$uri"
}

function build_gradle_uri() {
  echo "$GRADLE_URI_TEMPLATE"
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

function clear_android_settings() {
  export ANDROID_HOME=""
  export JAVA_HOME=""
  export ANDROID_SDK_MANAGER=""
  export GRADLE_HOME=""

}

function android_install_sdk_manager() {
  if [[ -n "$ANDROID_SDK_MANAGER" ]]; then
    return 0
  fi
    
  if ! prompt_yn "The Android SDK manager is required for building Android apps. Install it now?"; then
    return 1
  fi

  echo "Please review the Android SDK Manager License by visiting the URL below and clicking "Download SDK Platform-Tools for "[Your OS]"
  echo "$ANDROID_PLATFORM_TOOLS_PAGE_URI"
  
  if ! prompt_yn "Do you consent to the Android SDK Manager License?"; then
    return 1
  fi

  local _ah=""
  prompt_new_path "Enter location for ANDROID_HOME" "${ANDROID_HOME_SEARCH_PATHS[0]}" _ah
  
  if [ -d "$_ah" ]; then
    echo "Created $_ah"
  else
    return 1
  fi


  echo "Downloading $uri..."
  uri="$(build_android_platform_tools_uri)"
  archive="$(download_to_tmp "$uri")"
  if [ -z "$archive" ]; then
    echo "Failed to download $uri"
    return 1
  fi

  echo "Extracting $archive to $_ah"
  unpack "$archive" "$_ah"
  rm "$archive"

  if ! prompt_yn "Android Command line tools is required for building Android apps. Install it now?"; then
    return 1
  fi

  uri="$(build_android_command_line_tools_uri)"
  echo "Please review the Android Command line tools License by visiting the URL below, locating ""Command line tools only"" and clicking $(basename "$uri")"
  echo "$ANDROID_PLATFORM_TOOLS_PAGE_URI"
  
  if ! prompt_yn "Do you consent to the Android Command line tools License?"; then
    return 1
  fi

  archive=""
  echo "Downloading $uri..."
  archive="$(download_to_tmp "$uri")"
  if [ -z "$archive" ]; then
    echo "Failed to download $uri"
    return 1
  fi

  echo "Extracting $archive to $_ah"
  unpack "$archive" "$_ah"
  rm "$archive"

  ANDROID_HOME="$_ah"
  # ANDROID_SDK_MANAGER=
  return 0
}

function android_first_time_experience_setup() {
  # populates global search path list
  get_android_default_search_paths
  android_install_sdk_manager
}
