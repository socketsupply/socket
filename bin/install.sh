#!/usr/bin/env bash

# shellcheck disable=SC2155

## Exit immediately if a command exits with a non-zero status
set -e

## Allow tracing bash execution
if [ -n "$BASH_TRACE" ]; then
  set -x
fi

## Global constant
declare VERSION_FILE="VERSION.txt"
declare PLATFORM_IOS="ios"
declare PLATFORM_ANDROID="android"
declare PLATFORM_DESKTOP="desktop"

## Global repositories
declare LIBUV_REPO="git@github.com:libuv/libuv.git"
declare LIBUDX_REPO="git@github.com:mafintosh/libudx.git"

## Global state
declare PREFIX
declare BUILD_PATH="$(pwd)/build"

## Global commands, may be null/empty and platform dependant
declare AR
declare LD
declare CC
declare CPP
declare CXX
declare LIPO
declare STRIP
declare RANLIB

## Global Xcode related constants, probably for iOS
declare XCODE_PATH
declare XCODE_PLATFORMS_PATH
declare XCODE_TOOLCHAIN_PATH

## Global iOS (for Xcode) related constants
declare IPHONEOS_DEPLOYMENT_TARGET="8.0"
declare IPHONEOS_MIN_SDKVERSION="8.0"
declare -a pids=()

##
# Polyfill `sudo(1)` for environments that do not have it,
# such as Git Bash for Windows, etc
##
if ! which sudo > /dev/null 2>&1; then
  function sudo () {
    "$@"
    return $?
  }
fi

##
# TODO
##
function onsignal () {
  for pid in "${pids[@]}"; do
    kill -9 "$pid" 2>/dev/null
  done
}

##
# Resolves a command piping stderr to `/dev/null` returning 0
# upon sucess, otherwise an error code exit statud
##
function resolve_command () {
  which "$@" 2>/dev/null
  return $?
}

##
# Predicate helper function to determine if a command exists in PATH
##
function command_exists () {
  resolve_command "$@" >/dev/null
}

##
# Log pretty output to stdout
##
function log () {
  echo -ne "• $*"
  echo
}

##
# Log pretty error to stderr
##
function error () {
  {
    echo -ne "x Error: $*"
    echo
  } >&2
  return 1
}

##
# Log pretty warning to stderr
##
function warn () {
  {
    echo -ne "• Warning: $*"
    echo
  } >&2
}

##
# Log pretty error to stderr and exit
##
function panic () {
  error "$@"
  exit 1
}

##
# Returns `0` if first argument is in array list ($2...N)
#
# $1      Element to check for
# $2...N  An array/list of values to check if $1 exists in.
#
# Returns 0 if $1 is in array, otherwise 1
##
function array_contains () {
  local element="$1"
  local item
  shift

  for item; do
    if [[ "$item" == "$element" ]]; then
      return 0
    fi
  done

  return 1
}

##
# Lists files in a directories
#
# $1..N    Directories to list files in
##
function list_files () {
  $(which find) "$@" 2>/dev/null
  return $?
}

##
# Various prechecks before `configure()`
#
# [$1="desktop"] Platform target enumeration
#
# Returns 0 upon success, otherwise an error code exit status
##
function prechecks () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"
  local -a required_commands=(git strip)

  if [ -z "$HOME" ]; then
    panic "\$HOME environment variable is not defined"
  fi

  if [ "$platform_target" == "$PLATFORM_IOS" ]; then
    required_commands+=("ar" "ld" "lipo" "glibtoolize" "ranlib" "xcrun" "xcode-select" "autoconf")
  fi

  # Verify required commands exist
  for command in "${required_commands[@]}"; do
    if ! command_exists "$command"; then
      if [ "$command" == "xcode-select" ]; then
        panic "Missing '$command' in PATH: Xcode needs to be installed from the Mac App Store."
      elif [ "$command" == "autoconf" ] && [ "$platform_target" == "$PLATFORM_IOS" ]; then
        panic "Missing '$command' in PATH: Try 'brew install automake'"
      else
        panic "Missing '$command' in PATH"
      fi
    fi
  done
}

##
# Various postchecks after `configure()`, and before `build()`
#
# [$1="desktop"] Platform target enumeration
#
# Returns 0 upon success, otherwise an error code exit status
##
function postchecks () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"

  if [ -z "$CXX" ]; then
    panic "Could not determine \$CXX environment variable"
  else
    warn "\$CXX environment variable not set, assuming '$CXX'"
  fi

  if [ -z "$PREFIX" ]; then
    panic "Could not determine \$PREFIX environment variable"
  fi
}

##
# Configure bootstrap setup, compiler, etc.
#
# [$1="desktop"] Platform target enumeration
#
# Returns 0 upon success, otherwise an error code exit status
##
function configure () {
  log "Configuring Socket SDK build"

  local platform_target="${1:-"$PLATFORM_DESKTOP"}"

  prechecks "$@" || return $?

  if [ -z "$CXX" ]; then
    CXX="$(resolve_command clang++)"
  fi

  if [ -z "$CXX" ]; then
    CXX="$(resolve_command g++)"
  fi

  if [ -z "$CXX" ]; then
    CXX="$(resolve_command cpp)"
  fi

  export AR="${AR:-"$(resolve_command ar)"}"
  export LD="${LD:-"$(resolve_command ld)"}"
  export CC="${CC:-"$(resolve_command cc)"}"
  export CXX
  export CPP="${CPP:-"$(resolve_command cpp)"}"
  export STRIP="${STRIP:-"$(resolve_command strip)"}"
  export RANLIB="${RANLIB:-"$(resolve_command ranlib)"}"

  export PREFIX=${PREFIX:-"/usr/local"}

  postchecks "$@" || return $?
}

##
#
##
function configure_dependencies () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"

  if [ "$platform_target"  == "$PLATFORM_IOS" ]; then
    export AR="$(xcrun -sdk iphoneos -find ar)"
    export LD="$(xcrun -sdk iphoneos -find ld)"
    export CC="$(xcrun -sdk iphoneos -find clang)"
    export CXX="$(xcrun -sdk iphoneos -find clang++)"
    #export CPP="$(xcrun -sdk iphoneos -find cpp)"
    export CPP="$CC -E"
    export LIPO="$(xcrun -sdk iphoneos -find lipo)"
    export STRIP="$(xcrun -sdk iphoneos -find strip)"
    export RANLIB="$(xcrun -sdk iphoneos -find ranlib)"

    export XCODE_PATH="/Applications/Xcode.app"
    export XCODE_PLATFORMS_PATH="$XCODE_PATH/Contents/Developer/Platforms"
    export XCODE_TOOLCHAIN_PATH="$XCODE_PATH/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

    export IPHONEOS_DEPLOYMENT_TARGET="8.0"
  fi
}

##
# Computes version hash from HEAD of Git history.
##
function get_version_hash () {
  git rev-parse --short HEAD || return $?
}

##
# Computes version from maintained `VERSION.txt` file.
##
function get_version () {
  cat "$VERSION_FILE" || return $?
}

##
# Computes correct system SSC framework directory
##
function get_system_ssc_framework_directory () {
  if [ -n "$LOCALAPPDATA" ]; then # Windows/GitBash/MSYS
    echo -e "$LOCALAPPDATA/Programs/socketsupply"
  else
    echo -e "$HOME/.config/socket-sdk"
  fi
}

##
# Computes SSC framework source files
##
function get_ssc_framework_sources () {
  list_files "src/" -not -name "cli.cc" -not -name "cli.hh"
}

##
# Computes SSC framework library files
##
function get_ssc_framework_libraries () {
  list_files "lib/"
}

##
#
##
function get_latest_xcode_sdk_platform_version () {
  local platform_type="$1"
  local default_version="$2"
  local sdks_path="$XCODE_PLATFORMS_PATH/$platform_type.platform/Developer/SDKS"
  local -a sdks=()

  read -r -d '' -a sdks < <(list_files "$sdks_path")

  if (( ${#sdks[@]} == 0 )); then
    echo "$default_version"
    return 0
  fi

  basename "${sdks[(( ${#sdks[@]} - 1 ))]}" | sed "s/$platform_type//g" | sed "s/.sdk//g"
}

##
#
##
function export_platform_compiler_flags () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"
  local platform_arch="$2"
  local platform_type="$3"

  if [ "$platform_target" == "$PLATFORM_IOS" ]; then
    local sdk_version="$(get_latest_xcode_sdk_platform_version iPhoneOS 8.0)"
    local -a shared_flags=(-arch "$platform_arch" -isysroot "$XCODE_PLATFORMS_PATH/$platform_type.platform/Developer/SDKs/$platform_type$sdk_version.sdk")

    CFLAGS="${shared_flags[*]} -miphoneos-version-min=$IPHONEOS_MIN_SDKVERSION"
    LDFLAGS="${shared_flags[*]}"
    CXXFLAGS="${shared_flags[*]}"
    CPPFLAGS="$CFLAGS"

    export CPATH="$XCODE_PLATFORMS_PATH/$platform_type.platform/Developer/SDKs/$platform_type$sdk_version.sdk/usr/include"
  fi

  export CFLAGS
  export LDFLAGS
  export CPPFLAGS
  export CXXFLAGS
}

##
# Compiles the `ssc(1)` command line utility program
##
function compile_ssc () {
  local -a sources=("src/cli.cc")
  local -a flags=()

  # Output 'ssc' to bin ## TODO(jwerle): output to build directory
  flags+=(-o bin/ssc)

  if [ -n "${CXXFLAGS[*]}" ]; then
    flags+=("${CXXFLAGS[@]}")
  fi

  if [ -n "${CXX_FLAGS[*]}" ]; then
    flags+=("${CXX_FLAGS[@]}")
  fi

  ## C++ standard, etc
  flags+=("-std=c++2a")

  # Compute version macro preprocessor definitions
  flags+=(-DVERSION_HASH="$(get_version_hash)")
  flags+=(-DVERSION="$(get_version)")

  log "Compiling 'ssc'"
  warn "Will execute: $CXX ${sources[*]} ${flags[*]}"
  if ! "$CXX" "${sources[@]}" "${flags[@]}"; then
    panic "Unable to build. See trouble shooting guide in the README.md file"
  fi

  log "Successfully compiled 'ssc'"
  return 0
}

##
# Compiles 'libuv' into build/
##
function compile_libuv () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"

  local -a archs=("arm64" "i386" "x86_64")
  local -a hosts=("arm" "${archs[1]}" "${archs[2]}")

  if ! test -d "$BUILD_PATH/libuv"; then
    if ! git clone --depth=1 "$LIBUV_REPO" "$BUILD_PATH/libuv"; then
      panic "Failed to clone $LIBUV_REPO"
    fi
  fi

  pushd . || return $?
  cd "$BUILD_PATH/libuv" || return $?

  if ! test -f configure; then
    sh autogen.sh || return $?
  fi

  if [ "$platform_target" == "$PLATFORM_IOS" ]; then
    # The order corresponds to archs/hosts
    local -a platforms=("iPhoneOS" "iPhoneSimulator" "iPhoneSimulator")
    local i=0
    for (( i = 0; i < ${#platforms[@]}; i++ )); do
      local platform="${platforms[$i]}"
      local arch="${archs[$i]}"
      local host="${hosts[$i]}"

      mkdir -p "$BUILD_PATH/$arch/"{lib,include}

      cp -rf "$BUILD_PATH/libuv" "$BUILD_PATH/$arch/libuv"
      cd "$BUILD_PATH/$arch/libuv"

      {
        export_platform_compiler_flags "$PLATFORM_IOS" "$arch" "$platform"
        if ! test -f Makefile; then
          ./configure --prefix="$BUILD_PATH/$arch" --host="$host-apple-darwin"
        fi

        make -j4 || return $?
        make install || return $?
      } &
      pids+=($!)
    done
  fi

  wait "${pids[@]}" || return $?
  popd

  mkdir -p "$BUILD_PATH/"{lib,include} || return $?

  $LIPO -create                      \
    "$BUILD_PATH/arm64/lib/libuv.a"  \
    "$BUILD_PATH/x86_64/lib/libuv.a" \
    "$BUILD_PATH/i386/lib/libuv.a"   \
    -output "$BUILD_PATH/lib/libuv.a" || return $?

  rm -rf "$BUILD_PATH"/include/uv*
  cp -rf "$BUILD_PATH"/libuv/include/* "$BUILD_PATH/include"

  return 0
}

##
#
# Compiles 'libudx' into build/
##
function compile_libudx () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"
  local -a archs=("arm64" "i386" "x86_64")
  local sources=()

  if ! test -d "$BUILD_PATH/libudx"; then
    if ! git clone --depth=1 "$LIBUDX_REPO" "$BUILD_PATH/libudx"; then
      panic "Failed to clone $LIBUDX_REPO"
    fi
  fi

  pushd . || return $?
  cd "$BUILD_PATH/libudx" || return $?

  read -r -d '' -a sources < <(git ls-files -- 'src/*.c' ':!:*io_win.c')

  if [ "$platform_target" == "$PLATFORM_IOS" ]; then
    # The order corresponds to archs/hosts
    local -a platforms=("iPhoneOS" "iPhoneSimulator" "iPhoneSimulator")
    local i=0
    for (( i = 0; i < ${#platforms[@]}; i++ )); do
      local platform="${platforms[$i]}"

      local arch="${archs[$i]}"
      cp -rf "$BUILD_PATH/libudx" "$BUILD_PATH/$arch/libudx"
      cd "$BUILD_PATH/$arch/libudx"

      mkdir -p "$BUILD_PATH/$arch/lib"
      mkdir -p "$BUILD_PATH/$arch/include"

      {
        export_platform_compiler_flags "$PLATFORM_IOS" "$arch" "$platform"
        for src in "${sources[@]}"; do
          # shellcheck disable=SC2086
          "$CC" -std=c99 $CXXFLAGS            \
            -I "$BUILD_PATH/include"          \
            -c "$BUILD_PATH/$arch/libudx/$src"
        done

        local objects=()
        read -r -d '' -a objects < <(list_files "$BUILD_PATH/$arch/libudx/"*.o)
        "$AR" rvs "$BUILD_PATH/$arch/lib/libudx.a" "${objects[@]}"
      } &
      pids+=($!)
    done
  fi

  wait "${pids[@]}" || return $?
  popd

  mkdir -p "$BUILD_PATH/"{lib,include} || return $?

  $LIPO -create                      \
    "$BUILD_PATH/arm64/lib/libudx.a"  \
    "$BUILD_PATH/x86_64/lib/libudx.a" \
    "$BUILD_PATH/i386/lib/libudx.a"   \
    -output "$BUILD_PATH/lib/libudx.a" || return $?

  rm -rf "$BUILD_PATH"/include/udx*
  cp -rf "$BUILD_PATH"/libudx/include/* "$BUILD_PATH/include"

  return 0
}

##
# Compiles possibly platform dependant dependencies.
##
function compile_dependencies () {
  local platform_target="${1:-"$PLATFORM_DESKTOP"}"
  shift

  if [ "$platform_target" == "$PLATFORM_IOS" ]; then
    compile_libuv "$PLATFORM_IOS" "$@" || return $?
    compile_libudx "$PLATFORM_IOS" "$@" || return $?
  fi

  return 0
}

##
# TODO
##
function install_dependencies () {
  local destination="$(get_system_ssc_framework_directory)"

  if ! mkdir -p "$destination/lib"; then
    panic "Failed to make destination lib directory: $destination"
  fi

  rm -rf "$destination/lib/"*.a
  rm -rf "$destination/include"
  cp -rf "$BUILD_PATH/lib/"*.a "$destination/lib"
  cp -rf "$BUILD_PATH/include/" "$destination/include"

  return 0
}

##
# Installs 'ssc(1)' command into PATH
##
function install_ssc_command () {
  local destination="$(get_system_ssc_framework_directory)"

  if ! mkdir -p "$destination/bin"; then
    panic "Failed to make destination bin directory: $destination"
  fi

  log "Installing 'ssc' into: $destination"
  ## TODO(jwerle): use build directory
  if ! sudo install -b "$(pwd)/bin/ssc" "$PREFIX/bin"; then
    panic "Failed to install 'ssc' command into $PREFIX/bin"
  fi

  return 0
}

##
# Installs SSC framework source files into library directory.
##
function install_ssc_sources_and_libraries () {
  local destination="$(get_system_ssc_framework_directory)"
  local libraries=()
  local sources=()
  local cwd="$(pwd)"

  read -r -d '' -a sources < <(get_ssc_framework_sources)
  read -r -d '' -a libraries < <(get_ssc_framework_libraries)

  if [ -n "$LOCALAPPDATA" ]; then
    destination="$LOCALAPPDATA/Programs/socketsupply"
  else
    destination="$HOME/.config/socket-sdk"
  fi

  if ! mkdir -p "$destination/"{lib,src}; then
    panic "Failed to make destination library/source directories: $destination"
  fi

  if ! test -d "$cwd/src"; then
    panic "./src/ directory missing in working directory"
  fi

  if ! test -d "$cwd/lib"; then
    panic "./lib/ directory missing in working directory"
  fi

  log "Copying sources to $destination/src"
  if ! cp -rf "${sources[@]}" "$destination/src"; then
    panic "Failed to copy source files to $destination/src"
  fi

  log "Copying libraries to $destination/lib"
  if ! cp -rf "${libraries[@]}" "$destination/lib"; then
    panic "Failed to copy source files to $destination/lib"
  fi

  return 0
}

##
# Installs 'ssc(1)' command and framework files into system.
##
function install_ssc () {
  log "Installing Socket SDK Framework and 'ssc(1)' command line program"

  install_ssc_command "$@" || return $?
  install_ssc_sources_and_libraries "$@" || return $?

  log "Finished installing Socket SDK Framework and 'ssc(1)'"
  log "Type 'ssc -h' for help"

  return 0
}

##
# Main entry function.
#
# [$1="desktop"] Platform target enumeration such as
#                "ios", "android", or "desktop" (default)
#
# Returns 0 upon success, otherwise an error code exit status
##
function main () {
  configure "$@" || return $?
  compile_ssc "$@" || return $?

  configure_dependencies "$@" || return $?
  compile_dependencies "$@" || return $?

  install_ssc "$@" || return $?
  install_dependencies || return $?
}

trap onsignal SIGINT SIGTERM
## Run the `main()` function
time main "$@"
