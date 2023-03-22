#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"

declare -A DEFAULT_ANDROID_HOME
DEFAULT_ANDROID_HOME[Darwin]=$HOME/Library/Android/sdk 
DEFAULT_ANDROID_HOME[Win32]=$LOCALAPPDATA\\Android\\Sdk
DEFAULT_ANDROID_HOME[Linux]=$HOME/Android/Sdk

declare ANDROID_SDK_MANAGER_DEFAULT_SEARCH_PATHS=(
  "cmdline-tools/latest/bin"
  # These paths may be invalid
  "cmdline-tools/bin"
  "tools/bin"
)

declare ANDROID_PLATFORM_TOOLS_URI_TEMPLATE="https://dl.google.com/android/repository/platform-tools-latest-{os}.zip"
declare ANDROID_PLATFORM_TOOLS_PAGE_URI="https://developer.android.com/studio/releases/platform-tools"
declare ANDROID_COMMAND_LINE_TOOLS_URI_TEMPLATE="https://dl.google.com/android/repository/commandlinetools-{os}-9477386_latest.zip"
declare ANDROID_STUDIO_PAGE_URI="https://developer.android.com/studio"
declare JDK_VERSION="19.0.2"
declare JDK_URI_TEMPLATE="https://download.java.net/java/GA/jdk$JDK_VERSION/fdb695a9d9064ad6b064dc6df578380c/7/GPL/openjdk-$JDK_VERSION""_""{os}-{arch}_bin.{format}"
declare GRADLE_URI_TEMPLATE="https://services.gradle.org/distributions/gradle-8.0.2-bin.zip"

# TODO(mribbons): ubuntu / apt libs: apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386 lib32z1 libbz2-1.0:i386

function get_android_default_search_paths() {
  host="$(host_os)"

  # Prepopulate search paths with any known paths so that they can be verified quickly
  ANDROID_HOME_SEARCH_PATHS=()
  if [ -n "$ANDROID_HOME" ]; then
    ANDROID_HOME_SEARCH_PATHS+=("$ANDROID_HOME")
  fi

  ANDROID_HOME_SEARCH_PATHS+=("${DEFAULT_ANDROID_HOME["$host"]}")

  ANDROID_SDK_MANAGER_SEARCH_PATHS=()
  if [ -n "$ANDROID_SDK_MANAGER" ]; then
    ANDROID_SDK_MANAGER_SEARCH_PATHS+=("$(dirname "$ANDROID_SDK_MANAGER")")
  fi

  ANDROID_SDK_MANAGER_SEARCH_PATHS+=("${ANDROID_SDK_MANAGER_DEFAULT_SEARCH_PATHS[@]}")

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

  # Only attempt default homes if $ANDROID_HOME not defined
  if [[ "$host" = "Darwin"  ]]; then
    JAVA_HOME_SEARCH_PATHS+=("$HOME/.local/bin")
    JAVA_HOME_SEARCH_PATHS+=("$HOME/Applications")
    JAVA_HOME_SEARCH_PATHS+=("$HOME/homebrew")
    JAVA_HOME_SEARCH_PATHS+=("/Applications")
  elif [[ "$host" = "Linux"  ]]; then
    ANDROID_HOME_SEARCH_PATHS+=("$HOME")
    JAVA_HOME_SEARCH_PATHS+=("$HOME/.local/bin")
    # Java should be unpacked alongside Android Studio
    JAVA_HOME_SEARCH_PATHS+=("$HOME")
    JAVA_HOME_SEARCH_PATHS+=("/opt")
  elif [[ "$host" = "Win32"  ]]; then
    JAVA_HOME_SEARCH_PATHS+=("$LOCALAPPDATA\Programs")
    JAVA_HOME_SEARCH_PATHS+=("$PROGRAMFILES")
    GRADLE_SEARCH_PATHS+=("$LOCALAPPDATA\Programs")
  fi

  export ANDROID_HOME_SEARCH_PATHS
  export JAVA_HOME_SEARCH_PATHS
  export GRADLE_SEARCH_PATHS
}

function test_javac_version() {
  javac=$1
  target_version=$2

  jc_v="$("$javac" --version 2>/dev/null)"; r=$?
  if [[ "$r" != "0" ]]; then
    return $r
  fi

  jc_v="${jc_v/javac\ /}"
  [[ -n $VERBOSE ]] && echo "Comparing $javac version: "$(version "$jc_v") "$(version "$target_version")"
  if [ "$(version "$jc_v")" -lt "$(version "$target_version")" ]; then
    return 1
  fi

  return 0
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
        [[ -n $VERBOSE ]] && echo "Checking $android_home_test/$sdk_man_test/sdkmanager$bat"
        if [[ -f "$android_home_test/$sdk_man_test/sdkmanager$bat" ]]; then
          _ah="$android_home_test"
          _sdk="$sdk_man_test/sdkmanager$bat"
          break
        fi
      fi
    done
  done

  temp=$(mktemp)
  for java_home_test in "${JAVA_HOME_SEARCH_PATHS[@]}"; do
    # Try initial search in bin of known location before attempting `find`
    [[ -n $VERBOSE ]] && echo "Checking $java_home_test/bin/javac$exe"
    if [[ -f "$java_home_test/bin/javac$exe" ]]; then

      test_javac_version "$java_home_test/bin/javac$exe" "$JDK_VERSION" ; r=$?
      if [[ "$r" == "0" ]]; then
        # subshell, output to file
        echo "$(dirname "$(dirname "$javac")")" > "$temp"
        echo "$java_home_test" > "$temp"
        break
      else
        [[ -n $VERBOSE ]] && echo "Ignoring a $java_home_test/bin/javac$exe $jc_v"
      fi
    fi

    [[ -n $VERBOSE ]] && echo "find $java_home_test -type f -name "javac$exe"' -print0 2>/dev/null | while IFS= read -r -d '' javac"
    find "$java_home_test" -type f -name "javac$exe" -print0 2>/dev/null | while IFS= read -r -d '' javac
    do
      # break doesn't work here, check that we don't have a result
      if [[ $(stat_size "$temp") == 0 ]]; then
        [[ -n $VERBOSE ]] && echo "Found $javac"
        test_javac_version "$javac" "$JDK_VERSION" ; r=$?
        if [[ "$r" == "0" ]]; then
          # subshell, output to file
          echo "$(dirname "$(dirname "$javac")")" > "$temp"
        else
          [[ -n $VERBOSE ]] && echo "Ignoring b $javac $jc_v"
        fi
      fi
    done
  done

  if [[ $(stat_size "$temp") != 0 ]]; then
    _jh=$(cat "$temp")
  fi

  echo -n > "$temp"

  for gradle_test in "${GRADLE_SEARCH_PATHS[@]}"; do
    [[ -n $VERBOSE ]] && echo "Checking $gradle_test/bin/gradle$bat"
    if [[ -f "$gradle_test/bin/gradle$bat" ]]; then
      echo "$gradle_test" > "$temp"
      break
    fi

    [[ -n $VERBOSE ]] && echo "find $gradle_test -type f -name 'gradle' -print0 2>/dev/null | while IFS= read -r -d '' gradle"
    find "$gradle_test" -type f -name "gradle$bat" -print0 2>/dev/null | while IFS= read -r -d '' gradle
    do
      # break doesn't work here, check that we don't have a result
      if [[ $(stat_size "$temp") == 0 ]]; then
        [[ -n $VERBOSE ]] && echo "Found $gradle"
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
    ANDROID_HOME="$(native_path "$_ah")"
    export ANDROID_HOME
  fi
  
  if [[ -n "$_sdk" ]]; then
    ANDROID_SDK_MANAGER="$(native_path "$_sdk")"
    export ANDROID_SDK_MANAGER
  fi
  
  if [[ -n "$_jh" ]]; then
    JAVA_HOME="$(native_path "$_jh")"
    echo "Set JAVA_HOME: $JAVA_HOME"
    export JAVA_HOME
  else
    unset JAVA_HOME
  fi

  
  if [[ -n "$_gh" ]]; then
    GRADLE_HOME="$(native_path "$_gh")"
    export GRADLE_HOME
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
    arm64|x86-64)
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
  ANDROID_HOME=$1
  NDK_VERSION=$2
  host=$3
  host_arch=$4
  arch=$5
  plusplus=$6
  echo "$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/$(android_host_platform "$host")-$(android_arch "$host_arch")/bin/clang$plusplus --target=$(android_arch "$arch")-linux-android$(android_eabi "$arch")"
}

function android_ar() {
  # Note that this one is not target arch specific
  ANDROID_HOME=$1
  NDK_VERSION=$2
  host=$3
  host_arch=$4
  echo "$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/$(android_host_platform "$host")-$(android_arch "$host_arch")/bin/llvm-ar"
}

function android_arch_includes() {
  #get abi specific includes and sysroot
  arch=$1
  include=(
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/$(android_arch "$arch")-linux-android$(android_eabi "$arch")"
    "-I$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include"
    "--sysroot=$ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/$(android_arch "$arch")-linux-android$(android_eabi "$arch")"
  )

  echo "${include[@]}"
}

function android_min_expected_static_lib_size() {
  lib=$1
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
  abis=(
    "arm64-v8a"
    "armeabi-v7a"
    "x86"
    "x86_64"
  )

  echo "${abis[@]}"
}
export ANDROID_DEPS_ERROR

declare cmd=""
declare exe=""
declare bat=""
if [[ "$host" == "Win32" ]]; then
  exe=".exe"
  cmd=".cmd"
  bat=".bat"
fi

declare ANDROID_PLATFORM="33"
declare NDK_VERSION="25.0.8775105"

export BUILD_ANDROID

if [[ -z $SSC_NO_ANDROID ]]; then
  if [[ -z $CI ]] || [[ -n $SSC_ANDROID_CI ]]; then
    BUILD_ANDROID=1
  fi
fi

# used for testing
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
    
  if ! prompt_yn "The Android SDK Manager is required for building Android apps. Install it now?"; then
    return 1
  fi

  echo "Please review the Android SDK Manager License by visiting the URL below and clicking "Download SDK Platform-Tools for "[Your OS]"
  echo "$ANDROID_PLATFORM_TOOLS_PAGE_URI"
  
  if ! prompt_yn "Do you consent to the Android SDK Manager License?"; then
    return 1
  fi

  local _ah=""
  if [ -n "$ANDROID_HOME" ] && [ -d "$ANDROID_HOME" ]; then
    echo "Using existing ANDROID_HOME: $ANDROID_HOME folder"
    _ah="$ANDROID_HOME"
  else
    prompt_new_path "Enter location for ANDROID_HOME" "${ANDROID_HOME_SEARCH_PATHS[0]}" _ah \
      "If you want to use an existing path as ANDROID_HOME, set it in an environment variable before running this script."
    
    if [ -d "$_ah" ]; then
      echo "Created $_ah"
    else
      echo "Failed to create $_ah"
      return 1
    fi
  fi

  echo "Downloading $uri..."
  uri="$(build_android_platform_tools_uri)"
  archive="$(download_to_tmp "$uri")"
  if [ -z "$archive" ]; then
    echo "Failed to download $uri"
    return 1
  fi

  echo "Extracting $archive to $_ah"
  if ! unpack "$archive" "$_ah"; then
    echo "Failed to unpack $archive"
    return 1
  fi
  rm "$archive"

  if ! prompt_yn "Android Command line tools is required for building Android apps. Install it now?"; then
    return 1
  fi

  uri="$(build_android_command_line_tools_uri)"
  echo "Please review the Android Command line tools License by visiting the URL below, locating ""Command line tools only"" and clicking $(basename "$uri")"
  echo "$ANDROID_STUDIO_PAGE_URI"
  
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
  if ! unpack "$archive" "$_ah"; then
    echo "Failed to unpack $archive"
    return 1
  fi
  rm "$archive"

  
  # The contents of cmdline-tools need to be in cmdline-tools/latest (Per sdkmanager: Either specify it explicitly with --sdk_root= or move this package into its expected location: <sdk>\cmdline-tools\latest\)
  mkdir -p "$_ah"/latest || exit 1
  mv "$(unix_path "$_ah/cmdline-tools/")"* "$(unix_path "$_ah/latest")" || exit 1
  mv "$(unix_path "$_ah/latest")" "$(unix_path "$_ah/cmdline-tools/")"|| exit 1

  ANDROID_HOME="$_ah"
  export ANDROID_HOME
  ANDROID_SDK_MANAGER="$(native_path "cmdline-tools/latest/bin/sdkmanager$bat")"
  export ANDROID_SDK_MANAGER

  return 0
}

function android_install_jdk() {
  if [[ -n "$JAVA_HOME" ]]; then
      return 0
  fi

  if ! prompt_yn "The Java Developement Kit is required for building Android apps. Install OpenJDK $JDK_VERSION now?"; then
    return 1
  fi

  prompt_new_path "Enter parent location for JAVA_HOME (The JDK will be extracted within this folder)" "${JAVA_HOME_SEARCH_PATHS[0]}" _jh \
    "!CAN_EXIST!"
  
  if [ ! -d "$(unix_path "$_jh")" ]; then
    echo "JAVA_HOME parent path doesn't exist $_jh"
    return 1
  fi

  uri="$(build_jdk_uri)"
  archive=""
  echo "Downloading $uri..."
  archive="$(download_to_tmp "$uri")"
  if [ "$?" != "0" ]; then
    echo "Failed to download $uri: $archive"
    return 1
  fi

  # Determine internal folder name so we can add it to JAVA_HOME
  archive_dir="$(get_top_level_archive_dir "$archive")"
  echo "Extracting $archive to $_jh"
  if ! unpack "$archive" "$_jh"; then
    echo "Failed to unpack $archive"
    return 1
  fi
  rm "$archive"
  JAVA_HOME="$(native_path "$(escape_path "$_jh")/$archive_dir")"
  export JAVA_HOME
  return 0
}

function android_install_gradle() {
  if [[ -n "$GRADLE_HOME" ]]; then
      return 0
  fi

  if ! prompt_yn "Gradle is required for building Android apps. Install Gradle now?"; then
    return 1
  fi

  prompt_new_path "Enter parent location for GRADLE_HOME (Gradle will be extracted within this folder)" "${GRADLE_SEARCH_PATHS[0]}" _gh \
    "!CAN_EXIST!"
  
  if [ ! -d "$(unix_path "$_gh")" ]; then
    echo "GRADLE_HOME parent path doesn't exist $_gh"
    return 1
  fi

  uri="$(build_gradle_uri)"
  archive=""
  echo "Downloading $uri..."
  archive="$(download_to_tmp "$uri")"
  if [ "$?" != "0" ]; then
    echo "Failed to download $uri: $archive"
    return 1
  fi

  # Determine internal folder name so we can add it to GRADLE_HOME
  archive_dir="$(get_top_level_archive_dir "$archive")"
  echo "Extracting $archive to $_gh"
  if ! unpack "$archive" "$_gh"; then
    echo "Failed to unpack $archive"
    return 1
  fi
  rm "$archive"
  GRADLE_HOME="$(native_path "$(escape_path "$_gh")/$archive_dir")"
  export GRADLE_HOME
  return 0
}

function android_first_time_experience_setup() {
  if 
  [[ -d "$ANDROID_HOME" ]] && [[ -f "$ANDROID_HOME/$ANDROID_SDK_MANAGER" ]] && 
  [[ -d "$JAVA_HOME" ]] && [[ -d "$GRADLE_HOME" ]] && 
  [[ -n "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" ]];
  then
    return 0
  fi
  # builds global search path list
  get_android_default_search_paths
  android_install_sdk_manager; sdk_result=$?
  android_install_jdk; jdk_result=$?
  android_install_gradle; gradle_result=$?

  if [[ -z "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" ]]; then
    if prompt_yn "Do you want to automatically accept all Android SDK Manager licenses?"; then
      yes="$(which "yes" 2>/dev/null)"
      # Store yes path in native format so it can be used by ssc later
      ANDROID_SDK_MANAGER_ACCEPT_LICENSES="$(native_path "$yes")"
      if [ ! -f "$yes" ]; then
        echo "not ok - yes binary not found, unable to accept Android SDK Manager licenses."
      fi
    else
      ANDROID_SDK_MANAGER_ACCEPT_LICENSES="n"
    fi
  fi

  write_env_data
  if (( sdk_result + jdk_result + gradle_result != 0 )); then
    echo "One or more Android build components failed to install."
    return 1
  fi

  return 0
}


export ANDROID_ENV_FLOW

function android_env_flow() {
  [[ -n "$ANDROID_ENV_FLOW" ]] && return 0

  ANDROID_ENV_FLOW=1

  if [[ -n $BUILD_ANDROID ]]; then
    read_env_data
    get_android_paths
    write_env_data
    if android_first_time_experience_setup; then

      # Move ANDROID_SDK_MANAGER_JAVA_OPTS into JAVA_OPTS temporarily. We can't store it in .ssc.env because it doesn't work with gradle
      OLD_JAVA_OPTS="$JAVA_OPTS"
      [[ -n "$ANDROID_SDK_MANAGER_JAVA_OPTS" ]] && JAVA_OPTS="$OLD_JAVA_OPTS $ANDROID_SDK_MANAGER_JAVA_OPTS"

      SDK_OPTIONS=""
      SDK_OPTIONS+="\"ndk;$NDK_VERSION\" "
      SDK_OPTIONS+="\"platform-tools\" "
      SDK_OPTIONS+="\"platforms;android-$ANDROID_PLATFORM\" "
      SDK_OPTIONS+="\"emulator\" "
      SDK_OPTIONS+="\"patcher;v4\" "
      SDK_OPTIONS+="\"system-images;android-$ANDROID_PLATFORM;google_apis;x86_64\" "
      SDK_OPTIONS+="\"system-images;android-$ANDROID_PLATFORM;google_apis;arm64-v8a\" "

      yes="echo"
      [[ -n "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" ]] && [[ "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" != "n" ]] && yes="$ANDROID_SDK_MANAGER_ACCEPT_LICENSES"
      yes="$(unix_path "$yes")"

      if [[ -n "$ANDROID_HOME" ]]; then
        [[ -n $VERBOSE ]] && echo "Installing android deps"
        [[ -n $VERBOSE ]] && echo "$(unix_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER")" "$SDK_OPTIONS"
        # Without eval, sdk manager says there is a syntax error
        eval "($(unix_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER") $SDK_OPTIONS)"
        [[ -n $VERBOSE ]] && echo "Running Android licensing process"
        [[ -n $VERBOSE ]] && echo "$yes | $(native_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER")" --licenses
        $yes | "$(native_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER")" --licenses
      fi

      JAVA_OPTS="$OLD_JAVA_OPTS"
    fi
  fi

  if [[ -z "$ANDROID_HOME" ]]; then
    echo "Android dependencies: ANDROID_HOME not set."
    ANDROID_DEPS_ERROR=1
  fi

  export NDK_BUILD="$ANDROID_HOME/ndk/$NDK_VERSION/ndk-build$cmd"
  if ! test -f "$NDK_BUILD"; then
    echo "Android dependencies: ndk-build not at $NDK_BUILD"
    ANDROID_DEPS_ERROR=1
  fi

  return $?
}
