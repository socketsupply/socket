#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"

declare Darwin=1
declare Linux=2
declare Win32=3
declare DEFAULT_ANDROID_HOME=()

DEFAULT_ANDROID_HOME[Darwin]="$HOME/Library/Android/sdk"
DEFAULT_ANDROID_HOME[Linux]="$HOME/Android/Sdk"
DEFAULT_ANDROID_HOME[Win32]="$LOCALAPPDATA\\Android\\Sdk"

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
declare GRADLE_VERSION="8.0.2"
declare GRADLE_URI_TEMPLATE="https://services.gradle.org/distributions/gradle-$GRADLE_VERSION-bin.zip"

# TODO(mribbons): ubuntu / apt libs: apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386 lib32z1 libbz2-1.0:i386

function get_android_default_search_paths() {
  write_log "h" "# Determining default Android search paths."
  local host="$(host_os)"

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
    GRADLE_SEARCH_PATHS=("$GRADLE_HOME")
  fi

  if [ -n "$HOME" ]; then
    GRADLE_SEARCH_PATHS+=("$HOME/.gradle")
  fi

  # Only attempt default homes if $ANDROID_HOME not defined
  if [[ "$host" = "Darwin"  ]]; then
    JAVA_HOME_SEARCH_PATHS+=("$HOME/.local/bin")

    # Add $PATH java to JAVA_HOME_SEARCH_PATHS
    # Don't add this as first entry, first entry used as default install path
    local java="$(readlink -f "$(which java 2>/dev/null)" 2>/dev/null)"
    if [ -n "$java" ]; then
      JAVA_HOME_SEARCH_PATHS+=("$(dirname "$(dirname "$java")")")
    fi
    
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

  write_log "h" "ok - ANDROID_HOME, JAVA_HOME, and GRADLE_HOME search paths configured."
}

function test_javac_version() {
  local javac=$1
  local target_version=$2
  local jc_v

  # Setting JAVA_HOME to /usr causes sdkmanager to hang
  if [[ "$javac" == "/usr/bin/javac" ]] && [[ "$(host_os)" == "Darwin" ]]; then
    write_log "h" "warn - Ignoring system java selector ($javac)"
    return 1
  fi

  jc_v="$("$javac" --version 2>/dev/null)"; r=$?
  if [[ "$r" != "0" ]]; then
    return $r
  fi

  jc_v="${jc_v/javac\ /}"
  write_log "d" "# Comparing $javac version: "$(version "$jc_v") "$(version "$target_version")"
  if [ "$(version "$jc_v")" -lt "$(version "$target_version")" ]; then
    return 1
  fi

  return 0
}

function test_gradle_version() {
  local gradle_path="$1"
  local target_version=$2
  # Can't run gradle if JAVA_HOME not already set, but version number is in path
  local version=""
  [[ "$gradle_path" =~ gradle-(.*) ]] && version="${BASH_REMATCH[1]}"

  write_log "d" "# Comparing $gradle_path version: $(version "$target_version")"
  if [ "$(version "$version")" -lt "$(version "$target_version")" ]; then
    return 1
  fi

  return 0
}

function validate_sdkmanager_jar() {
  local sdkmanager_path="$1"
  # Currently sdkmanager as shipped with Android Studio is broken
  local set=""
  [[ "$(host_os)" == "Win32" ]] && set="set "
  local pattern="^""$set""CLASSPATH\="
  local classpath_line="$(grep "$pattern" "$sdkmanager_path")"
  # This indicates a cmdline-tools version of sdkmanager
  if [[ "$classpath_line" == *"sdkmanager-classpath.jar"* ]]; then
    return 0
  fi
  return 1;
}

function get_android_paths() {
  get_android_default_search_paths
  write_log "h" "# Determining Android paths."

  local _ah
  local _sdk
  local _jh
  local _gh
  local android_home_test
  local sdk_man_test
  local bat="$(use_bin_ext ".bat")"
  local exe="$(use_bin_ext ".exe")"
  local sdkmanager_path

  for android_home_test in "${ANDROID_HOME_SEARCH_PATHS[@]}"; do
    for sdk_man_test in "${ANDROID_SDK_MANAGER_SEARCH_PATHS[@]}"; do
      sdkmanager_path="$android_home_test/$sdk_man_test/sdkmanager$bat"
      write_log "v" "# Checking $sdkmanager_path"
      if [[ -f "$sdkmanager_path" ]]; then
        if validate_sdkmanager_jar "$sdkmanager_path"; then
          if [[ -z "$_ah" ]]; then
            _ah="$android_home_test"
          fi
          _sdk="$sdk_man_test/sdkmanager$bat"
          break
        else
          echo "warn - can't use $sdkmanager_path: Android Studio's SDK manager script is inoperable."
        fi
      fi
    done
  done

  local temp
  local java_home_test
  local javac

  temp=$(mktemp)
  for java_home_test in "${JAVA_HOME_SEARCH_PATHS[@]}"; do
    # Try initial search in bin of known location before attempting `find`
    javac="$(unix_path "$java_home_test" "1")/bin/javac$exe"
    write_log "v" "# Checking $(native_path "$javac")"
    if [[ -f "$javac" ]]; then

      test_javac_version "$javac" "$JDK_VERSION" ; r=$?
      if [[ "$r" == "0" ]]; then
        # subshell, output to file
        echo "$(dirname "$(dirname "$javac")")" > "$temp"
        write_log "h" "# Using predetermined javac $(native_path "$javac")"
        break
      else
        write_log "v" "# Ignoring predetermined javac $(native_path "$javac") $jc_v"
        # configured JAVA_HOME is bad, unset to trigger new version install
        [[ "$JAVA_HOME" == "$java_home_test" ]] && unset JAVA_HOME
      fi
    else
      write_log "v" "No javac at $javac"
    fi

    if ! test -d "$_jh" ; then
      write_log "d" "# find $java_home_test -type f -name "javac$exe"' -print0 2>/dev/null | while IFS= read -r -d '' javac"
      # break doesn't work here, check that we don't have a result
      if [[ $(stat_size "$temp") == 0 ]]; then
        find "$java_home_test" -type f -name "javac$exe" -print0 2>/dev/null | while IFS= read -r -d '' javac; do
          write_log "v" "# Found $javac"
          test_javac_version "$javac" "$JDK_VERSION" ; r=$?
          if [[ "$r" == "0" ]]; then
            # subshell, output to file
            write_log "h" "# Using found javac $javac"
            echo "$(dirname "$(dirname "$javac")")" > "$temp"
          else
            write_log "v" "# Ignoring found javac $javac $jc_v"
          fi
        done
      fi
    fi
  done

  if [[ $(stat_size "$temp") != 0 ]]; then
    _jh=$(cat "$temp")
  fi

  echo -n > "$temp"

  local gradle_test
  local gradle

  for gradle_test in "${GRADLE_SEARCH_PATHS[@]}"; do
    write_log "d" "find $gradle_test -type f -name 'gradle' -print0 2>/dev/null | while IFS= read -r -d '' gradle"
    find "$gradle_test" -type f -name "gradle$bat" -print0 2>/dev/null | while IFS= read -r -d '' gradle; do
      # break doesn't work here, check that we don't have a result
      if [[ $(stat_size "$temp") == 0 ]]; then
        local gradle_path=$(dirname "$(dirname "$gradle")")
        test_gradle_version "$gradle_path" "$GRADLE_VERSION" ; r=$?
        if [[ "$r" == "0" ]]; then
          write_log "h" "ok - Found gradle ($gradle_path)"
          # subshell, output to file
          echo "$gradle_path" > "$temp"
        else
          write_log "v" "# Ignoring found gradle ($gradle_path)"
        fi
      fi
    done
  done

  if [[ $(stat_size "$temp") != 0 ]]; then
    _gh=$(cat "$temp")
  fi
  rm -f "$temp"

  if [[ -n "$_jh" ]]; then
    JAVA_HOME="$(native_path "$_jh")"
    write_log "h" "ok - Found JAVA_HOME ($JAVA_HOME)"
    export JAVA_HOME
  fi

  if [[ -n "$_gh" ]]; then
    GRADLE_HOME="$(native_path "$_gh")"
    write_log "h" "ok - Found GRADLE_HOME ($GRADLE_HOME)"
    export GRADLE_HOME
  fi

  if [[ -n "$_ah" ]]; then
    ANDROID_HOME="$(native_path "$_ah")"
    write_log "h" "ok - Found ANDROID_HOME ($ANDROID_HOME)"
    export ANDROID_HOME
  fi

  if [[ -n "$_sdk" ]]; then
    ANDROID_SDK_MANAGER="$(native_path "$_sdk")"
    write_log "h" "ok - Found sdkmanager ($ANDROID_SDK_MANAGER)"
    export ANDROID_SDK_MANAGER
  fi
}

function build_android_platform_tools_uri() {
  echo "${ANDROID_PLATFORM_TOOLS_URI_TEMPLATE/\{os\}/$(android_host_platform "$(host_os)")}"
}

function build_android_command_line_tools_uri() {
  local os="$(lower "$(host_os)")"

  if [[ "$os" == "darwin" ]]; then
    os="mac"
  elif [[ "$os" == "win32" ]]; then
    os="win"
  fi

  echo "${ANDROID_COMMAND_LINE_TOOLS_URI_TEMPLATE/\{os\}/$os}"
}


function build_jdk_uri() {
  local os="$(lower "$(host_os)")"
  local arch=$(uname -m)
  local format="tar.gz"

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
export ANDROID_DEPS_ERROR

declare ANDROID_PLATFORM="33"
declare NDK_VERSION="25.0.8775105"

export BUILD_ANDROID

if [[ -z $NO_ANDROID ]]; then
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

  write_log "h" "# Please review the Android SDK Manager License by visiting the URL below and clicking "Download SDK Platform-Tools for "[Your OS]"
  write_log "h" "# $ANDROID_PLATFORM_TOOLS_PAGE_URI"

  if ! prompt_yn "Do you consent to the Android SDK Manager License?"; then
    return 1
  fi

  local _ah=""
  if [ -n "$ANDROID_HOME" ] && [ -d "$ANDROID_HOME" ]; then
    write_log "h" "# Using existing ANDROID_HOME: $ANDROID_HOME folder"
    _ah="$ANDROID_HOME"
  else
    prompt_new_path "Enter location for ANDROID_HOME" "${ANDROID_HOME_SEARCH_PATHS[0]}" _ah \
      "If you want to use an existing path as ANDROID_HOME, set it in an environment variable before running this script."

    if [ -d "$_ah" ]; then
      write_log "d" "ok - Created $_ah"
    else
      write_log "h" "not ok - Failed to create $_ah"
      return 1
    fi
  fi

  write_log "h" "# Downloading $uri..."
  local uri="$(build_android_platform_tools_uri)"
  local archive="$(download_to_tmp "$uri")"
  if [ -z "$archive" ]; then
    write_log "h" "not ok - Failed to download $uri"
    return 1
  fi

  write_log "h" "# Extracting $archive to $_ah"
  if ! unpack "$archive" "$_ah"; then
    write_log "h" "not ok - Failed to unpack $archive"
    return 1
  fi
  rm -f "$archive"

  if ! prompt_yn "Android Command line tools is required for building Android apps. Install it now?"; then
    return 1
  fi

  uri="$(build_android_command_line_tools_uri)"
  write_log "h" "# Please review the Android Command line tools License by visiting the URL below, locating ""Command line tools only"" and clicking $(basename "$uri")"
  write_log "h" "# $ANDROID_STUDIO_PAGE_URI"

  if ! prompt_yn "Do you consent to the Android Command line tools License?"; then
    return 1
  fi

  archive=""
  write_log "h" "# Downloading $uri..."
  archive="$(download_to_tmp "$uri")"
  if [ -z "$archive" ]; then
    write_log "h" "# Failed to download $uri"
    return 1
  fi

  write_log "h" "# Extracting $archive to $_ah"
  if ! unpack "$archive" "$_ah"; then
    write_log "h" "# Failed to unpack $archive"
    return 1
  fi
  rm -f "$archive"


  # The contents of cmdline-tools need to be in cmdline-tools/latest (Per sdkmanager: Either specify it explicitly with --sdk_root= or move this package into its expected location: <sdk>\cmdline-tools\latest\)
  mkdir -p "$_ah"/latest || exit 1
  mv "$(unix_path "$_ah/cmdline-tools/")"* "$(unix_path "$_ah/latest")" || exit 1
  mv "$(unix_path "$_ah/latest")" "$(unix_path "$_ah/cmdline-tools/")"|| exit 1

  ANDROID_HOME="$_ah"
  export ANDROID_HOME
  ANDROID_SDK_MANAGER="$(native_path "cmdline-tools/latest/bin/sdkmanager$(use_bin_ext ".bat")")"
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

  local _jh
  prompt_new_path "Enter parent location for JAVA_HOME (The JDK will be extracted within this folder)" "${JAVA_HOME_SEARCH_PATHS[0]}" _jh \
    "!CAN_EXIST!"

  if [ ! -d "$(unix_path "$_jh")" ]; then
    write_log "h" "# JAVA_HOME parent path doesn't exist $_jh"
    return 1
  fi

  local uri="$(build_jdk_uri)"
  local archive=""
  write_log "h" "# Downloading $uri..."
  archive="$(download_to_tmp "$uri")"
  if [ "$?" != "0" ]; then
    write_log "h" "# Failed to download $uri: $archive"
    return 1
  fi

  # Determine internal folder name so we can add it to JAVA_HOME
  local archive_dir="$(get_top_level_archive_dir "$archive")"
  write_log "h" "# Extracting $archive to $_jh"
  if ! unpack "$archive" "$_jh"; then
    write_log "h" "# Failed to unpack $archive"
    return 1
  fi
  rm -f "$archive"
  [[ "$(host_os)" == "Darwin" ]] && os_specific_path="/Contents/Home"
  JAVA_HOME="$(native_path "$(escape_path "$_jh")/$archive_dir$os_specific_path")"
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

  local _gh
  prompt_new_path "Enter parent location for GRADLE_HOME (Gradle will be extracted within this folder)" "${GRADLE_SEARCH_PATHS[0]}" _gh \
    "!CAN_EXIST!"

  if [ ! -d "$(unix_path "$_gh")" ]; then
    write_log "h" "# GRADLE_HOME parent path doesn't exist $_gh"
    return 1
  fi

  local uri="$(build_gradle_uri)"
  archive=""
  write_log "h" "# Downloading $uri..."
  archive="$(download_to_tmp "$uri")"
  if [ "$?" != "0" ]; then
    write_log "h" "# Failed to download $uri: $archive"
    return 1
  fi

  # Determine internal folder name so we can add it to GRADLE_HOME
  archive_dir="$(get_top_level_archive_dir "$archive")"
  write_log "h" "# Extracting $archive to $_gh"
  if ! unpack "$archive" "$_gh"; then
    write_log "h" "# Failed to unpack $archive"
    return 1
  fi
  rm -f "$archive"
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
  local sdk_result
  local jdk_result
  local gradle_result
  local yes
  local PROMPT_DEFAULT_YN

  if [[ -n "$pass_yes_deps" ]]; then
    PROMPT_DEFAULT_YN="y"
    export PROMPT_DEFAULT_YN
  fi

  if [[ "$(host_os)" != "Win32" ]] && _android_setup_required && ! prompt_yn "Do you want to install Android build dependencies?
Download size: 5.5GB, Installed size: 12.0GB"; then
    return 2
  fi

  get_android_default_search_paths

  write_log "h" "# Installing Android SDK manager."
  android_install_sdk_manager; sdk_result=$?
  (( sdk_result )) && write_log "h" "not ok - Failed to install Android SDK manager."

  write_log "h" "# Installing JDK."
  android_install_jdk; jdk_result=$?
  (( jdk_result )) && write_log "h" "not ok - Failed to install JDK."

  write_log "h" "# Installing Gradle."
  android_install_gradle; gradle_result=$?
  (( gradle_result )) && write_log "h" "not ok - Failed to install Gradle."

  if [[ -z "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" ]]; then
    if prompt_yn "Do you want to automatically accept all Android SDK Manager licenses?"; then
      yes="$(which "yes" 2>/dev/null)"
      # Store yes path in native format so it can be used by ssc later
      ANDROID_SDK_MANAGER_ACCEPT_LICENSES="$(native_path "$yes")"
      if [ ! -f "$yes" ]; then
        write_log "h" "not ok - yes binary not found, unable to accept Android SDK Manager licenses."
      fi
    else
      ANDROID_SDK_MANAGER_ACCEPT_LICENSES="n"
      yes="echo"
    fi
  fi

  if [[ -n "$pass_yes_deps" ]]; then
    unset PROMPT_DEFAULT_YN
  fi

  write_log "v" "# Writing detected environment data."
  write_env_data
  if (( sdk_result + jdk_result + gradle_result != 0 )); then
    write_log "h" "not ok - One or more Android build components failed to install."
    return 1
  fi

  return 0
}

function android_setup_required() {
  if _android_setup_required; then
    exit 1
  fi

  exit 0
}

function _android_setup_required() {
  read_env_data
  if
    [[ -z "$ANDROID_HOME" ]] || [[ ! -d "$ANDROID_HOME" ]] ||
    [[ ! -f "$ANDROID_HOME/$ANDROID_SDK_MANAGER" ]] ||
    [[ -z "$JAVA_HOME" ]] || [[ ! -d "$JAVA_HOME" ]];
  then
    return 0
  fi

  return 1
}

export android_fte

function android_fte() {
  pass_yes_deps=""
  # use 'exit' instead of return
  # fixes "not ok - Android setup failed." when setup is valid
  local set_exit_code=""

  [[ -n "$android_fte" ]] && return 0

  while (( $# > 0 )); do
    declare arg="$1"; shift
    [[ "$arg" == "--yes-deps" ]] && pass_yes_deps="$arg"
    [[ "$arg" == "--exit-code" ]] && set_exit_code="1"
  done

  android_fte=1

  if (( BUILD_ANDROID )); then
    read_env_data
    get_android_paths
    write_env_data

    android_first_time_experience_setup; rc=$?

    if (( rc == 2 )); then
      echo "warn - You have elected to not install Android dependencies."
      echo "warn - You can use 'export NO_ANDROID=1' to avoid this prompt in the future."
      unset BUILD_ANDROID
      return $rc
    fi

    if (( rc == 0 )); then
      # Move ANDROID_SDK_MANAGER_JAVA_OPTS into JAVA_OPTS temporarily. We can't store JAVA_OPTS in .ssc.env because it doesn't work with gradle
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

      local yes="echo"
      [[ -n "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" ]] && [[ "$ANDROID_SDK_MANAGER_ACCEPT_LICENSES" != "n" ]] && yes="$ANDROID_SDK_MANAGER_ACCEPT_LICENSES"
      yes="$(unix_path "$yes")"

      if [[ -n "$ANDROID_HOME" ]]; then
        write_log "h" "# Ensuring Android dependencies are installed"
        write_log "d" "$yes | $(unix_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER")" "$SDK_OPTIONS"
        # Without eval, sdk manager says there is a syntax error
        eval "$yes | ($(unix_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER") $SDK_OPTIONS)"
        write_log "v" "# Running Android licensing process"
        write_log "d" "$yes | $(native_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER")" --licenses
        $yes | "$(native_path "$ANDROID_HOME/$ANDROID_SDK_MANAGER")" --licenses
      fi

      JAVA_OPTS="$OLD_JAVA_OPTS"
    fi
  fi

  if [[ -z "$ANDROID_HOME" ]]; then
    write_log "d" "# Android dependencies: ANDROID_HOME not set."
    ANDROID_DEPS_ERROR=1
  fi

  NDK_BUILD="$ANDROID_HOME/ndk/$NDK_VERSION/ndk-build$(use_bin_ext ".cmd")"
  export NDK_BUILD

  rc=$?
  [[ -n "$set_exit_code" ]] && exit $rc
  return $rc
}

function main() {
  while (( $# > 0 )); do
    declare arg="$1"; shift
    [[ "$arg" == "--android-fte" ]] && android_fte "$@"
    # [[ "$arg" == "--android-fte" ]] && exit 0
    [[ "$arg" == "--android-setup-required" ]] && android_setup_required "$@"
  done
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
fi
