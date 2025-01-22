#!/usr/bin/env bash

declare root=""

root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"
source "$root/bin/android-functions.sh"

if [[ -z "$CPU_CORES" ]]; then
  export CPU_CORES=$(set_cpu_cores)
fi

if [[ -n $VERBOSE ]]; then
  echo "# using cores: $CPU_CORES"
fi

if [[ -n "$NO_ANDROID" ]]; then
  unset BUILD_ANDROID
fi

if [[ -z "$NO_IOS" ]]; then
  BUILD_IOS=1
fi

declare arch="$(host_arch)"
declare args=()
declare pids=()
declare force=0
declare pass_force=""
declare pass_ignore_header_mtimes=""
declare host="$(host_os)"

LIPO=""
declare CWD=$(pwd)
declare PREFIX="${PREFIX:-"/usr/local"}"
declare BUILD_DIR="$CWD/build"

export BUILDING_SSC_CLI=1

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
    pass_force="$arg"
    force=1; continue
  fi

  if [[ "$arg" = "--yes-deps" ]] || [[ "$arg" = "-y" ]]; then
    pass_yes_deps="$arg"; continue
  fi

  if [[ "$arg" == "--no-android-fte" ]]; then
    no_android_fte=1; continue
  fi

  # Don't rebuild if header mtimes are newer than .o files - Be sure to manually delete affected assets as required
  if [[ "$arg" == "--ignore-header-mtimes" ]]; then
    pass_ignore_header_mtimes="$arg"
    ignore_header_mtimes=1; continue
  fi

  args+=("$arg")
done

if [ -n "$LOCALAPPDATA" ] && [ -z "$SOCKET_HOME" ]; then
  SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
else
  SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"
fi

if [[ "$host" == "Win32" ]] && [[ "$PREFIX" == "/usr/local" ]] && [[ ! -d "$PREFIX/bin" ]]; then
  # User probably doesn't want to install to /usr/local on windows. Reset PREFIX so script doesn't terminate later.
  PREFIX="$SOCKET_HOME"
fi

write_log "h" "warn - using '$SOCKET_HOME' as SOCKET_HOME"
write_log "h" "warn - Installing to '$PREFIX'"

if [[ "$host" = "Linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    echo "not ok - WSL is not supported."
    exit 1
  fi
fi

if [ -z "$SOCKET_HOME" ]; then
  if [ -n "$LOCALAPPDATA" ]; then
    SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
  fi
fi

declare pass_ignore_header_mtimes=""

declare d=""
if [[ "$host" == "Win32" ]]; then
  # We have to differentiate release and debug for Win32
  # Problem:
  # When building libuv and socket-runtime with debug enabled, our apps crash when
  # using ifstream:
  # `Debug Assertion Failed. Expression: (_osfile(fh) & fopen)`
  # This build issue also prevents debugging in Visual Studio.

  # This occurs because by default clang incorrectly links to the non
  # threaded, production version of C runtime .lib (libcrt)
  # After taking the nessary steps to manually link to the correct lib
  # (Including adding preprocessor definitions for /MT[d]), see ldflags.sh under Win32
  # ssc and apps won't link, therefore:

  # Solution:
  # Splits debug and release build artifacts:
  # d is set if $DEBUG and $host == Win32
  # The file[d].lib suffix is commonly used within the Windows SDK to differentiate debug and non debug files
  # In Visual Studio, Debug profiles usually have to be manually modified to include eg ole32d.lib instead ole32.lib.
  # I have used this convention to separate debug objects and libs where possible
  # *.o files are now named *$d.o
  # Libs are copied to build/platform/lib$d (libuv.lib didn't support being renamed, this would require modification of the build chain)
  # libsocket-runtime.a is now named libsocket-runtime$d.a
  # ssc build --prod defines whether or not the app is being built for debug
  # and therefore links to the app being built to the correct libsocket-runtime$d.a
  if [[ -n "$DEBUG" ]]; then
    d="d"
  fi
fi

determine_cxx || exit $?
read_env_data

declare package_manager="$(determine_package_manager)"

function advice {
  local sudo="sudo ";
  [[ "$package_manager" = "brew install" ]] && sudo=""
  echo "$sudo""$package_manager $1"
}

if [[ "$host" != "Win32" ]]; then
  if ! quiet command -v sudo; then
    sudo () {
      $@
      return $?
    }
  fi
fi

if [[ "$(uname -s)" != *"_NT"* ]]; then
  quiet command -v make
  die $? "not ok - missing build tools, try \"$(advice "make")\""
fi

if [ "$host" == "Darwin" ]; then
  quiet command -v automake
  die $? "not ok - missing build tools, try \"$(advice "automake")\""
  quiet command -v glibtoolize
  die $? "not ok - missing build tools, try \"$(advice "libtool")\""
  quiet command -v libtool
  die $? "not ok - missing build tools, try \"$(advice "libtool")\""
  quiet command -v curl
  die $? "not ok - missing curl, try \"$(advice "curl")\""
  if ! brew list | grep libomp >/dev/null 2>&1; then
    die $? "not ok - missing libomp, try \"$(advice "libomp")\""
  fi
fi

if [ "$host" == "Linux" ]; then
  quiet command -v autoconf
  die $? "not ok - missing build tools, try \"$(advice "autoconf")\""
  quiet command -v pkg-config
  die $? "not ok - missing pkg-config tool, \"$(advice 'pkg-config')\""
  quiet command -v libtoolize
  die $? "not ok - missing build tools, try \"$(advice "libtool")\""
  quiet command -v curl
  die $? "not ok - missing curl, \"$(advice 'curl')\""
fi

if [[ -n "$BUILD_ANDROID" ]] && [[ "arm64" == "$(host_arch)" ]] && [[ "Linux" == "$host" ]]; then
  echo "warn - Android not supported on "$host"-"$(uname -m)", will unset BUILD_ANDROID"
fi

if [[ -n "$no_android_fte" ]] && [[ -z "$ANDROID_HOME" ]]; then
  unset BUILD_ANDROID
fi

if [[ -n "$BUILD_ANDROID" ]] && [[ "arm64" == "$(host_arch)" ]] && [[ "Linux" == "$host" ]]; then
  echo "warn - Android not supported on "$host"-"$(uname -m)", will unset BUILD_ANDROID"
  unset BUILD_ANDROID
fi

if [[ -n "$no_android_fte" ]] && [[ -z "$ANDROID_HOME" ]]; then
  unset BUILD_ANDROID
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  android_fte "$pass_yes_deps" && rc=$?
  # android_fte will unset BUILD_ANDROID if user elects not to install
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  abis=($(android_supported_abis))
  platform="android"
  clang="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$(host_arch)")"

  if ! quiet "$clang" -v; then
    echo "not ok - Android clang call failed. This could indicate an issue with ANDROID_HOME, missing ndk tools, or incorrectly determined host or target architectures."
    exit 1
  fi
fi

function _build_cli {
  local arch="$(host_arch)"
  local platform="desktop"
  local src="$root/src"
  local output_directory="$BUILD_DIR/$arch-$platform"

  echo "# building cli for desktop ($arch)..."

  # Expansion won't work under _NT
  # uv found by -L
  # referenced directly below
  # local libs=($("echo" -l{socket-runtime}))
  local libs=""

  #
  # Add libuv, socket-runtime and llama
  #
  if [[ "$(uname -s)" != *"_NT"* ]]; then
    libs=($("echo" -l{uv,llama,socket-runtime}))
  fi

  if [[ -n "$VERBOSE" ]]; then
    echo "# cli libs: ${libs[@]}, $(uname -s)"
  fi

  local ldflags=($("$root/bin/ldflags.sh" --arch "$arch" --platform $platform ${libs[@]}))
  local cflags=(-DSSC_CLI $("$root/bin/cflags.sh"))

  local test_headers=()
  if [[ -z "$ignore_header_mtimes" ]]; then
    test_headers+=("$(find "$src"/cli/*.hh 2>/dev/null)")
  fi
  test_headers+=("$src"/../VERSION.txt)
  local newest_mtime=0
  newest_mtime="$(latest_mtime ${test_headers[@]})"

  local win_static_libs=()
  local static_libs=()
  local test_sources=($(find "$src"/cli/*.cc 2>/dev/null))
  local sources=()
  local outputs=()

  mkdir -p "$BUILD_DIR/$arch-$platform/bin"
  local build_ssc=0

  for source in "${test_sources[@]}"; do
    local output="${source/$src/$output_directory}"
    # For some reason cli causes issues when debug and release are in the same folder
    output="${output/.cc/$d.o}"
    output="${output/cli/cli$d}"
    if (( force )) || ! test -f "$output" || (( newest_mtime > $(stat_mtime "$output") )) || (( newest_mtime > $(stat_mtime "$output") )) || (( $(stat_mtime "$source") > $(stat_mtime "$output") )); then
      sources+=("$source")
      outputs+=("$output")
      build_ssc=1
    fi
  done

  for (( i = 0; i < ${#sources[@]}; i++ )); do
    mkdir -p "$(dirname "${outputs[$i]}")"
    quiet "$CXX" "${cflags[@]}"  \
      -c "${sources[$i]}"      \
      -o "${outputs[$i]}"
    die $? "$CXX ${cflags[@]} -c \"${sources[$i]}\" -o \"${outputs[$i]}\""
  done

  local exe=""
  local static_libs=""
  local test_sources=($(find "$BUILD_DIR/$arch-$platform"/cli$d/*$d.o 2>/dev/null))
  if [[ "$(uname -s)" == *"_NT"* ]]; then
    declare d=""
    if [[ -n "$DEBUG" ]]; then
      d="d"
    fi
    exe=".exe"
    win_static_libs+=("$BUILD_DIR/$arch-$platform/lib$d/libsocket-runtime$d.a")
    test_sources+=("$static_libs")
  elif [[ "$(uname -s)" == "Linux" ]]; then
    static_libs+=("$BUILD_DIR/$arch-$platform/lib/libuv.a")
    static_libs+=("$BUILD_DIR/$arch-$platform/lib/libllama.a")
    static_libs+=("$BUILD_DIR/$arch-$platform/lib/libsocket-runtime.a")
  elif [[ "$(uname -s)" == "Darwin" ]]; then
    cflags+=("-fopenmp")
  fi

  libs=($(find "$root/build/$arch-$platform/lib$d/*" 2>/dev/null))
  test_sources+=(${libs[@]})
  local ssc_output="$BUILD_DIR/$arch-$platform/bin/ssc$exe"

  for source in "${test_sources[@]}"; do
    if (( force )) || (( build_ssc )) || ! test -f "$ssc_output" || (( $(stat_mtime "$source") > $(stat_mtime "$ssc_output") )); then
      build_ssc=1
      # break
    fi
  done

  if (( build_ssc )); then
    #
    # TODO "$static_libs" where it was doesn't work, if windows requires it to
    # be where it was, there should be a separate branch for windows.
    #
    quiet "$CXX"                                 \
      "${win_static_libs[@]}"                    \
      "$BUILD_DIR/$arch-$platform"/cli$d/*$d.o   \
      "${static_libs[@]}"                        \
      "${cflags[@]}"                             \
      "${ldflags[@]}"                            \
      -o "$ssc_output"

    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} \"${ldflags[@]}\" -o \"$BUILD_DIR/$arch-$platform/bin/ssc\""
    echo "ok - built the cli for desktop"
  fi
}

function _build_runtime_library() {
  local arch="$(host_arch)"
  echo "# building runtime library"
  "$root/bin/build-runtime-library.sh" --arch "$arch" --platform desktop $pass_force $pass_ignore_header_mtimes & pids+=($!)

  if [[ "$host" = "Darwin" ]] && [[ -z "$NO_IOS" ]]; then
    "$root/bin/build-runtime-library.sh" --arch "$arch" --platform ios $pass_force $pass_ignore_header_mtimes & pids+=($!)
    "$root/bin/build-runtime-library.sh" --arch x86_64 --platform ios-simulator $pass_force $pass_ignore_header_mtimes & pids+=($!)
    if [[ "$arch" = "arm64" ]] && [[ -z "$NO_IOS" ]]; then
      "$root/bin/build-runtime-library.sh" --arch "$arch" --platform ios-simulator $pass_force $pass_ignore_header_mtimes & pids+=($!)
    fi
  fi

  if [[ -n "$BUILD_ANDROID" ]]; then
    for abi in $(android_supported_abis); do
      "$root/bin/build-runtime-library.sh" --platform android --arch "$abi" $pass_force $pass_ignore_header_mtimes & pids+=($!)
    done
  fi

  wait
}

function _get_web_view2() {
  if [[ "$(uname -s)" != *"_NT"* ]] && [ -z "$FORCE_WEBVIEW2_DOWNLOAD" ]; then
    return
  fi

  local arch="$(host_arch)"
  local platform="desktop"

  if [ -z "$FORCE_WEBVIEW2_DOWNLOAD" ] && test -f "$BUILD_DIR/$arch-$platform/lib$d/WebView2LoaderStatic.lib"; then
    echo "$BUILD_DIR/$arch-$platform/lib$d/WebView2LoaderStatic.lib exists."
    return
  fi

  local tmp=$(mktemp -d)
  local pwd=$(pwd)

  echo "# Downloading Webview2"

  curl -L https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2592.51 --output "$tmp/webview2.zip"
  cd "$tmp" || exit 1
  unzip -q "$tmp/webview2.zip"
  mkdir -p "$BUILD_DIR/include"
  mkdir -p "$BUILD_DIR/$arch-$platform/lib$d"/

  cp -pf build/native/include/WebView2.h "$BUILD_DIR/include/WebView2.h"
  cp -pf build/native/include/WebView2EnvironmentOptions.h "$BUILD_DIR/include/WebView2EnvironmentOptions.h"
  cp -pf build/native/x64/WebView2LoaderStatic.lib "$BUILD_DIR/$arch-$platform/lib$d/WebView2LoaderStatic.lib"

  cd "$pwd"

  rm -rf "$tmp"
}

function _prebuild_desktop_main () {
  echo "# precompiling main program for desktop"
  local arch="$(host_arch)"
  local platform="desktop"

  local src="$root/src"
  local objects="$BUILD_DIR/$arch-$platform/objects"

  local test_headers=()
  if [[ -z "$ignore_header_mtimes" ]]; then
    test_headers+=("$(find "$src"/**/*.hh)")
  fi
  local newest_mtime=0
  newest_mtime="$(latest_mtime ${test_headers[@]})"

  local cflags=($("$root/bin/cflags.sh"))
  local test_sources=($(find "$src"/desktop/*.{cc,mm} 2>/dev/null))
  local sources=()
  local outputs=()

  mkdir -p "$(dirname "$output")"

  for source in "${test_sources[@]}"; do
    local output="${source/$src/$objects}"
    output="${output/.cc/$d.o}"
    output="${output/.mm/$d.o}"
    if (( force )) || ! test -f "$output" || (( newest_mtime > $(stat_mtime "$output") )) || (( $(stat_mtime "$source") > $(stat_mtime "$output") )); then
      sources+=("$source")
      outputs+=("$output")
    fi
  done

  for (( i = 0; i < ${#sources[@]}; i++ )); do
    mkdir -p "$(dirname "${outputs[$i]}")"
    quiet "$CXX" "${cflags[@]}" \
      -c "${sources[$i]}"       \
      -o "${outputs[$i]}"
    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} -c ${sources[$i]} -o ${outputs[$i]}"
  done

  echo "ok - precompiled main program for desktop"
}

function _prebuild_ios_main () {
  echo "# precompiling main program for iOS"
  local arch="arm64"
  local platform="iPhoneOS"

  local src="$root/src"
  local objects="$BUILD_DIR/$arch-$platform/objects"

  local clang="$(xcrun -sdk iphoneos -find clang++)"
  local cflags=($(TARGET_OS_IPHONE=1 "$root/bin/cflags.sh"))
  local test_sources=($(find "$src"/ios/*.mm 2>/dev/null))
  local sources=()
  local outputs=()

  mkdir -p "$objects"

  for source in "${test_sources[@]}"; do
    local output="${source/$src/$objects}"
    output="${output/.cc/$d.o}"
    output="${output/.mm/$d.o}"
    if (( force )) || ! test -f "$output" || (( $(stat_mtime "$source") > $(stat_mtime "$output") )); then
      sources+=("$source")
      outputs+=("$output")
    fi
  done

  for (( i = 0; i < ${#sources[@]}; i++ )); do
    mkdir -p "$(dirname "${outputs[$i]}")"
    "$clang" "${cflags[@]}" \
      -c "${sources[$i]}"   \
      -o "${outputs[$i]}"
    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} -c ${sources[$i]} -o ${outputs[$i]}"
  done
  echo "ok - precompiled main program for iOS"
}

function _prebuild_ios_simulator_main () {
  echo "# precompiling main program for iOS Simulator"
  local arch="$1"
  local platform="iPhoneSimulator"

  local src="$root/src"
  local objects="$BUILD_DIR/$arch-$platform/objects"

  local clang="$(xcrun -sdk iphonesimulator -find clang++)"
  local cflags=($(TARGET_IPHONE_SIMULATOR=1 ARCH="$arch" $root/bin/cflags.sh))
  local test_sources=($(find "$src"/ios/*.mm 2>/dev/null))
  local sources=()
  local outputs=()

  mkdir -p "$objects"

  for source in "${test_sources[@]}"; do
    local output="${source/$src/$objects}"
    output="${output/.cc/$d.o}"
    output="${output/.mm/$d.o}"
    if (( force )) || ! test -f "$output" || (( $(stat_mtime "$source") > $(stat_mtime "$output") )); then
      sources+=("$source")
      outputs+=("$output")
    fi
  done

  for (( i = 0; i < ${#sources[@]}; i++ )); do
    mkdir -p "$(dirname "${outputs[$i]}")"
    quiet "$clang" "${cflags[@]}" \
      -c "${sources[$i]}"         \
      -o "${outputs[$i]}"
    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$clang ${cflags[@]} -c \"${sources[$i]}\" -o \"${outputs[$i]}\""
  done
  echo "ok - precompiled main program for iOS Simulator ($arch)"
}

function _prepare {
  echo "# preparing directories..."
  local arch="$(host_arch)"
  rm -rf "$SOCKET_HOME"/{lib$d,src,bin,include,objects,api,pkgconfig}
  rm -rf "$SOCKET_HOME"/{lib$d,objects}/"$arch-desktop"

  mkdir -p "$SOCKET_HOME"/{lib$d,src,bin,include,objects,api,pkgconfig}
  mkdir -p "$SOCKET_HOME"/{lib$d,objects}/"$arch-desktop"

  if [[ "$host" = "Darwin" ]]; then
    mkdir -p "$SOCKET_HOME"/{lib$d,objects}/{arm64-iPhoneOS,x86_64-iPhoneSimulator,arm64-iPhoneSimulator}
  fi

  if [[ -n $BUILD_ANDROID ]]; then
    for abi in $(android_supported_abis); do
      mkdir -p "$SOCKET_HOME"/{lib$d,objects}/"$abi"
    done
  fi

  if [ ! -d "$BUILD_DIR/uv" ]; then
    git clone --depth=1 https://github.com/socketsupply/libuv.git "$BUILD_DIR/uv" > /dev/null 2>&1
    rm -rf $BUILD_DIR/uv/.git
    # Comment out compiler tests, we've covered these sufficiently for now
    if [[ -z "$ENABLE_LIBUV_C_COMPILER_CHECKS" ]]; then
      tempmkl=$(mktemp)
      sed 's/check_c_compiler_flag/# check_c_compiler_flag/' "$BUILD_DIR/uv/CMakeLists.txt" > "$tempmkl"
      mv "$tempmkl" "$BUILD_DIR/uv/CMakeLists.txt"
    fi

    die $? "not ok - unable to clone libuv. See trouble shooting guide in the README.md file"
  fi

  if [ ! -d "$BUILD_DIR/llama" ]; then
    git clone --depth=1 https://github.com/socketsupply/llama.cpp.git "$BUILD_DIR/llama" > /dev/null 2>&1
    # rm -rf $BUILD_DIR/llama/.git

    die $? "not ok - unable to clone llama. See trouble shooting guide in the README.md file"
  fi

  echo "ok - directories prepared"
}

function _install {
  local arch="$1"
  local platform="$2"

  if [ "$platform" == "desktop" ]; then
    echo "# copying sources to $SOCKET_HOME/src"
    cp -r "$CWD"/src/* "$SOCKET_HOME/src"
    if [[ "$arch" = "aarch64" ]]; then
      arch="arm64"
    fi
  fi

  # TODO(@heapwolf): Set lib types based on platform, after mobile CI is working

  if test -d "$BUILD_DIR/$arch-$platform/objects"; then
    echo "# copying objects to $SOCKET_HOME/objects/$arch-$platform"
    rm -rf "$SOCKET_HOME/objects/$arch-$platform"
    mkdir -p "$SOCKET_HOME/objects/$arch-$platform"
    cp -rfp "$BUILD_DIR/$arch-$platform/objects"/* "$SOCKET_HOME/objects/$arch-$platform"
  fi

  if test -d "$BUILD_DIR/lib$d"; then
    echo "# copying libraries to $SOCKET_HOME/lib$d"
    mkdir -p "$SOCKET_HOME/lib$d"
    cp -rfp "$BUILD_DIR"/lib$d/*.a "$SOCKET_HOME/lib$d/"
  fi

  _d=$d

  if [[ "$platform" == "android" ]]; then
    # Debug builds not currently supported for android
    _d=""
  fi

  if test -d "$BUILD_DIR/$arch-$platform"/lib$_d; then
    echo "# copying libraries to $SOCKET_HOME/lib$_d/$arch-$platform"
    rm -rf "$SOCKET_HOME/lib$_d/$arch-$platform"
    mkdir -p "$SOCKET_HOME/lib$_d/$arch-$platform"

    if [[ "$platform" != "android" ]]; then
      cp -rfp "$BUILD_DIR/$arch-$platform"/lib$_d/*.a "$SOCKET_HOME/lib$_d/$arch-$platform"
      if [[ "$host" == "Darwin" ]] && [[ "$platform" != "desktop" ]]; then
        cp -rfp "$BUILD_DIR/$arch-$platform"/lib/*.metallib "$SOCKET_HOME/lib/$arch-$platform"
      fi
    fi
    if [[ "$host" == "Win32" ]] && [[ "$platform" == "desktop" ]]; then
      cp -rfp "$BUILD_DIR/$arch-$platform"/lib$_d/*.lib "$SOCKET_HOME/lib$_d/$arch-$platform"
    fi

    if [[ "$platform" == "android" ]] && [[ -d "$BUILD_DIR/$arch-$platform"/lib ]]; then
      cp -fr "$BUILD_DIR/$arch-$platform"/lib/*.a "$SOCKET_HOME/lib/$arch-$platform"
    fi
  else
    echo >&2 "not ok - Missing $BUILD_DIR/$arch-$platform/lib"
    exit 1
  fi

  if [ "$platform" == "desktop" ]; then
    if [ "$host" == "Linux" ] || [ "$host" == "Darwin" ]; then
      echo "# copying pkgconfig to $SOCKET_HOME/pkgconfig"
      rm -rf "$SOCKET_HOME/pkgconfig"
      mkdir -p "$SOCKET_HOME/pkgconfig"
      cp -rfp "$BUILD_DIR/$arch-desktop/pkgconfig"/* "$SOCKET_HOME/pkgconfig"
    fi

    echo "# copying js api to $SOCKET_HOME/api"
    mkdir -p "$SOCKET_HOME/api"
    cp -frp "$root"/api/* "$SOCKET_HOME/api"

    mkdir -p "$SOCKET_HOME/assets"
    cp -rf "$root"/assets/* "$SOCKET_HOME/assets"

    # only do this for desktop, no need to copy again for other platforms
    rm -rf "$SOCKET_HOME/include"
    mkdir -p "$SOCKET_HOME/include"
    cp -rfp "$BUILD_DIR"/uv/include/* "$SOCKET_HOME/include"
    cp -rfp "$root"/include/* "$SOCKET_HOME/include"
    rm -f "$SOCKET_HOME/include/socket/_user-config-bytes.hh"

    mkdir -p "$SOCKET_HOME/include/llama"
    for header in $(find "$root/build/llama" -name *.h); do
      if [[ "$header" =~  examples/ ]]; then continue; fi
      if [[ "$header" =~  tests/ ]]; then continue; fi

      local llama_build_dir="$root/build/llama/"
      local destination="$SOCKET_HOME/include/llama/${header/$llama_build_dir/}"

      mkdir -p "$(dirname "$destination")"
      cp -f "$header" "$destination"
    done

    if [[ -f "$root/$SSC_ENV_FILENAME" ]]; then
      if [[ -f "$SOCKET_HOME/$SSC_ENV_FILENAME" ]]; then
        echo "# warn - Won't overwrite $SOCKET_HOME/$SSC_ENV_FILENAME"
      else
        echo "# copying $SSC_ENV_FILENAME to $SOCKET_HOME"
        cp -fp "$root/$SSC_ENV_FILENAME" "$SOCKET_HOME/$SSC_ENV_FILENAME"
      fi
    else
      echo "warn - $SSC_ENV_FILENAME not created."
    fi
  fi

  if [ "$platform" == "desktop" ]; then
    mkdir -p "$SOCKET_HOME/bin"
    # Required for FTE setup
    cp -ap "$root/bin/functions.sh" "$SOCKET_HOME/bin"
    cp -ap "$root/bin/android-functions.sh" "$SOCKET_HOME/bin"

    if [[ "$(uname -s)" == *"_NT"* ]]; then
      cp -ap "$root/bin/"*.ps1 "$SOCKET_HOME/bin"
      cp -ap "$root/bin/".vs* "$SOCKET_HOME/bin"
    fi
  fi

}

function _install_cli {
  local arch="$(host_arch)"

  if [ -z "$TEST" ] && [ -z "$NO_INSTALL" ]; then
    echo "# moving binary to '$SOCKET_HOME/bin' (prompting to copy file into directory)"

    cp -f "$BUILD_DIR/$arch-desktop"/bin/* "$SOCKET_HOME/bin"
    die $? "not ok - unable to move binary into '$SOCKET_HOME'"

    if [[ "$SOCKET_HOME" != "$PREFIX" ]]; then
      if [[ ! -d $PREFIX/bin ]]; then
        echo "not ok - $PREFIX/bin is not a directory, unable to install."
        exit 1
      fi

      echo "# linking binary to $PREFIX/bin/ssc"
      local status="$(ln -sf "$SOCKET_HOME/bin/ssc" "$PREFIX/bin/ssc" 2>&1)"
      local rc=$?

      if [[ " $status " =~ " Permission denied " ]]; then
        echo "warn - Failed to link binary to '$PREFIX/bin': Trying 'sudo'"
        sudo rm -f "$PREFIX/bin/ssc"
        sudo ln -sf "$SOCKET_HOME/bin/ssc" "$PREFIX/bin/ssc"
        die $? "not ok - unable to link binary into '$PREFIX/bin'"
      fi

      die $rc "not ok - unable to link binary into '$PREFIX/bin'"
    fi

    echo "ok - done. type 'ssc -h' for help"
  else
    echo "ok - done."
  fi
}

function _setSDKVersion {
  sdks=$(ls "$PLATFORMPATH"/"$1".platform/Developer/SDKs)
  arr=()
  for sdk in $sdks
  do
    echo "ok - found SDK $sdk"
    arr[${#arr[@]}]=$sdk
  done

  # Last item will be the current SDK, since it is alpha ordered
  count=${#arr[@]}

  if [ $count -gt 0 ]; then
    sdk=${arr[$count-1]:${#1}}
    num=$(expr ${#sdk}-4)
    SDKVERSION=${sdk:0:$num}
  else
    SDKVERSION="8.0"
  fi
}

function _compile_libuv_android {
  local platform="android"
  local arch=$1
  local host_arch="$(host_arch)"
  local clang="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch")"
  local clang_target="$(android_clang_target "$arch")"
  local ar="$(android_ar "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch")"
  local android_includes=$(android_arch_includes "$arch")

  local cflags=("$clang_target" -std=gnu89 -g -pedantic -I"$root"/build/uv/include -I"$root"/build/uv/src -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGEFILE_SOURCE -fPIC -Wall -Wextra -Wno-pedantic -Wno-sign-compare -Wno-unused-parameter -Wno-implicit-function-declaration)
  cflags+=("${android_includes[*]}")
  local objects=()
  local sources=("unix/async.c" "unix/core.c" "unix/dl.c" "unix/fs.c" "unix/getaddrinfo.c" "unix/getnameinfo.c" "unix/linux.c" "unix/loop.c" "unix/loop-watcher.c" "unix/pipe.c" "unix/poll.c" "unix/process.c" "unix/proctitle.c" "unix/random-devurandom.c" "unix/random-getentropy.c" "unix/random-getrandom.c" "unix/random-sysctl-linux.c" "unix/signal.c" "unix/stream.c" "unix/tcp.c" "unix/thread.c" "unix/tty.c" "unix/udp.c" fs-poll.c idna.c inet.c random.c strscpy.c strtok.c threadpool.c timer.c uv-common.c uv-data-getter-setters.c version.c)

  local output_directory="$root/build/$arch-$platform/uv$d"
  mkdir -p "$output_directory"

  local src_directory="$root/build/uv/src"

  trap onsignal INT TERM
  local i=0
  local max_concurrency=$CPU_CORES
  local build_static=0
  local base_lib="libuv"
  local static_library="$root/build/$arch-$platform/lib/$base_lib.a"

  for source in "${sources[@]}"; do
    if (( i++ > max_concurrency )); then
      for pid in "${pids[@]}"; do
        wait "$pid" 2>/dev/null
      done
      i=0
    fi

    declare object="${source/.c/.o}"
    object="$(basename "$object")"
    objects+=("$output_directory/$object")

    {
      if (( force )) || ! test -f "$output_directory/$object" || (( $(stat_mtime "$src_directory/$source") > $(stat_mtime "$output_directory/$object") )); then
        mkdir -p "$(dirname "$object")"
        echo "# compiling object ($arch-$platform) $(basename "$source")"
        quiet $clang "${cflags[@]}" -c "$src_directory/$source" -o "$output_directory/$object" || onsignal
        echo "ok - built $source -> $object ($arch-$platform)"
        # Can't write back to variable in block, remove final library to force rebuild
        rm "$static_library" 2>/dev/null
      fi
    } & pids+=($!)
  done

  for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null
  done

  if [ ! -f "$static_library" ]; then
    build_static=1
  fi
  mkdir -p "$(dirname "$static_library")"

  if (( build_static )); then
    quiet $ar crs "$static_library" "${objects[@]}"
    if [ -f "$static_library" ]; then
      echo "ok - built $base_lib ($arch-$platform): $(basename "$static_library")"
    else
       echo >&2 "not ok - failed to build $static_library"
      exit 1
    fi

  else
    if [ -f "$static_library" ]; then
      echo "ok - using cached static library ($arch-$platform): $(basename "$static_library")"
    else
      echo >&2 "not ok - static library doesn't exist after cache check passed: ($arch-$platform): $(basename "$static_library")"
      exit 1
    fi
  fi

  # This is a sanity check to confirm that the static_library is > 8 bytes
  # If an empty ${objects[@]} is provided to ar, it will still spit out a header without an error code.
  # therefore check the output size
  # This error condition should only occur after a code change
  local lib_size="$(stat_size "$static_library")"
  if (( lib_size < $(android_min_expected_static_lib_size "$base_lib") )); then
    echo >&2 "not ok - $static_library size looks wrong: $lib_size, renaming as .bad"
    mv "$static_library" "$static_library.bad"
    exit 1
  fi
}

function _compile_llama_metal {
  local target=$1
  local hosttarget=$1
  local platform=$2

  if [ -z "$target" ]; then
    target="$(host_arch)"
    platform="desktop"
  fi

  echo "# building METAL for $platform ($target) on $host..."
  local STAGING_DIR="$BUILD_DIR/$target-$platform/llama"

  if [ ! -d "$STAGING_DIR" ]; then
    mkdir -p "$STAGING_DIR"
    cp -r "$BUILD_DIR"/llama/* "$STAGING_DIR"
    cd "$STAGING_DIR" || exit 1
  else
    cd "$STAGING_DIR" || exit 1
  fi

  local sdk="iphoneos"
  [[ "$platform" == "iPhoneSimulator" ]] && sdk="iphonesimulator"

  mkdir -p "$STAGING_DIR/build/"
  mkdir -p ../lib

  xcrun -sdk $sdk metal -O3 -c ggml/src/ggml-metal/ggml-metal.metal -o ggml-metal.air
  xcrun -sdk $sdk metallib ggml-metal.air -o ../lib/default.metallib
  rm *.air

  echo "ok - metal built for $platform"
}

function _compile_llama {
  local target=$1
  local hosttarget=$1
  local platform=$2

  if [ -z "$target" ]; then
    target="$(host_arch)"
    platform="desktop"
  fi

  echo "# building llama.cpp for $platform ($target) on $host..."
  local STAGING_DIR="$BUILD_DIR/$target-$platform/llama"

  if [ ! -d "$STAGING_DIR" ]; then
    mkdir -p "$STAGING_DIR"
    cp -r "$BUILD_DIR"/llama/* "$STAGING_DIR"
    cd "$STAGING_DIR" || exit 1
  else
    cd "$STAGING_DIR" || exit 1
  fi

  local sdk="iphoneos"
  [[ "$platform" == "iPhoneSimulator" ]] && sdk="iphonesimulator"

  mkdir -p "$STAGING_DIR/build/"
  mkdir -p ../bin

  local cmake_args=(
    -DLLAMA_BUILD_TESTS=OFF
    -DLLAMA_BUILD_SERVER=OFF
    -DLLAMA_BUILD_EXAMPLES=OFF
    -DBUILD_SHARED_LIBS=OFF
  )

  if [ "$platform" == "desktop" ]; then
    if [[ "$host" != "Win32" ]]; then
      quiet command -v cmake
      die $? "not ok - missing cmake, \"$(advice 'cmake')\""

      quiet cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/$target-$platform" ${cmake_args[@]}
      die $? "not ok - libllama.a (desktop)"

      quiet cmake --build build &&
      quiet cmake --build build -- -j"$CPU_CORES" &&
      quiet cmake --install build
      die $? "not ok - libllama.a (desktop)"
    else
      if ! test -f "$BUILD_DIR/$target-$platform/lib$d/llama.lib"; then
        local config="Release"
        if [[ -n "$DEBUG" ]]; then
          config="Debug"
        fi
        cd "$STAGING_DIR/build/" || exit 1
        quiet command -v cmake
        die $? "not ok - missing cmake, \"$(advice 'cmake')\""
        quiet cmake -S .. -B . ${cmake_args[@]}
        quiet cmake --build . --config $config
        mkdir -p "$BUILD_DIR/$target-$platform/lib$d"
        quiet echo "cp -up $STAGING_DIR/build/$config/llama.lib "$BUILD_DIR/$target-$platform/lib$d/llama.lib""
        cp -up "$STAGING_DIR/build/$config/llama.lib" "$BUILD_DIR/$target-$platform/lib$d/llama.lib"
        if [[ -n "$DEBUG" ]]; then
          cp -up "$STAGING_DIR"/build/$config/llama_a.pdb "$BUILD_DIR/$target-$platform/lib$d/llama_a.pdb"
        fi;
      fi
    fi

    rm -f "$root/build/$(host_arch)-desktop/lib$d"/*.{so,la,dylib}*
    return
  elif [ "$platform" == "iPhoneOS" ] || [ "$platform" == "iPhoneSimulator" ]; then
    # https://github.com/ggerganov/llama.cpp/discussions/4508
    local ar="$(xcrun -sdk $sdk -find ar)"

    local cc="$(xcrun -sdk $sdk -find clang)"
    local cxx="$(xcrun -sdk $sdk -find clang++)"
    local cflags="--target=$target-apple-ios -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -m$sdk-version-min=$SDKMINVERSION -DLLAMA_METAL_EMBED_LIBRARY=ON -DUSE_NEON_DOTPROD "
    if [ "$platform" == "iPhoneOS" ]; then
      cflags+="-march=armv8.2-a+dotprod"
    elif [ "$platform" == "iPhoneSimulator" ] && [ "$target" == "arm64" ]; then
      cflags+="-march=armv8.2-a+dotprod"
    elif [ "$platform" == "iPhoneSimulator" ] && [ "$target" == "x86_64" ]; then
      cflags+="-march=x86-64 -target=x86-apple-ios-simulator"
    fi

    export AR="$ar"
    export CFLAGS="$cflags"
    export CXXFLAGS="$cflags"
    export CXX="$cxx"
    export CC="$cc"
    export SDKROOT="$PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk"

    cmake -S . -B build -DCMAKE_SYSTEM_NAME="iOS" -DCMAKE_OSX_ARCHITECTURES="$target" -DCMAKE_OSX_SYSROOT="$SDKROOT" -DCMAKE_C_COMPILER="$cc" -DCMAKE_CXX_COMPILER="$cxx" -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/$target-$platform" -DLLAMA_NATIVE=OFF -DGGML_ARM_DOTPROD=ON ${cmake_args[@]} &&
    cmake --build build &&
    cmake --build build -- -j"$CPU_CORES" &&
    cmake --install build

    if (( $? != 0 )); then
      die $? "not ok - Unable to compile libllama for '$platform'"
      return
    fi
    return
  elif [ "$platform" == "android" ]; then
    if [[ "$host" == "Win32" ]]; then
      echo "WARN - Building libllama for Android on Windows is not yet supported"
      return
    else
      local android_includes=$(android_arch_includes "$1")
      local host_arch="$(host_arch)"
      local cc="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch")"
      local cxx="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch" "++")"
      local clang_target="$(android_clang_target "$target")"
      local ar="$(android_ar "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch")"
      local cflags=("$clang_target" -std=c++2a -g -pedantic "${android_includes[*]}")

      AR="$ar" CFLAGS="$cflags" CXXFLAGS="$cflags" CXX="$cxx" CC="$cc" make UNAME_S="Android" UNAME_M=".." UNAME_P="$1" LLAMA_FAST=1 libllama.a

      if [ ! $? = 0 ]; then
        die $? "not ok - Unable to compile libllama for '$platform'"
        return
      fi
    fi
  fi

  if [[ "$host" != "Win32" ]]; then
    cp libllama.a ../lib
    die $? "not ok - Unable to compile libllama for '$platform'"
  fi

  cd "$BUILD_DIR" || exit 1
  rm -f "$root/build/$target-$platform/lib$d"/*.{so,la,dylib}*
  echo "ok - built libllama for $target-$platform"
  return 0
}

function _compile_libuv {
  local target=$1
  local hosttarget=$1
  local platform=$2

  if [ -z "$target" ]; then
    target="$(host_arch)"
    platform="desktop"
  fi

  echo "# building libuv for $platform ($target) on $host..."
  local STAGING_DIR="$BUILD_DIR/$target-$platform/uv"

  if [ ! -d "$STAGING_DIR" ]; then
    mkdir -p "$STAGING_DIR"
    cp -r "$BUILD_DIR"/uv/* "$STAGING_DIR"
    cd "$STAGING_DIR" || exit 1
    # Doesn't work in mingw
    if [[ "$host" != "Win32" ]]; then
      quiet sh autogen.sh
    fi;
  else
    cd "$STAGING_DIR" || exit 1
  fi

  mkdir -p "$STAGING_DIR/build/"

  if [ "$platform" == "desktop" ]; then
    if [[ "$host" != "Win32" ]]; then
      if ! test -f Makefile; then
      if [[ "$host" == "Linux" ]]; then
        CFLAGS="-fPIC" quiet ./configure --prefix="$BUILD_DIR/$target-$platform"
        die $? "not ok - desktop configure"
      else
        quiet ./configure --prefix="$BUILD_DIR/$target-$platform"
        die $? "not ok - desktop configure"
      fi

        quiet make
        quiet make "-j$CPU_CORES"
        quiet make install
      fi
    else
      if ! test -f "$BUILD_DIR/$target-$platform/lib$d/libuv.lib"; then
        local config="Release"
        if [[ -n "$DEBUG" ]]; then
          config="Debug"
        fi
        cd "$STAGING_DIR/build/" || exit 1
        quiet command -v cmake
        die $? "not ok - missing cmake, \"$(advice 'cmake')\""
        quiet cmake .. -DBUILD_TESTING=OFF -DLIBUV_BUILD_SHARED=OFF
        cd "$STAGING_DIR" || exit 1
        quiet cmake --build "$STAGING_DIR/build/" --config $config
        mkdir -p "$BUILD_DIR/$target-$platform/lib$d"
        quiet echo "cp -up $STAGING_DIR/build/$config/libuv.lib "$BUILD_DIR/$target-$platform/lib$d/libuv.lib""
        cp -up "$STAGING_DIR/build/$config/libuv.lib" "$BUILD_DIR/$target-$platform/lib$d/libuv.lib"
        if [[ -n "$DEBUG" ]]; then
          cp -up "$STAGING_DIR"/build/$config/uv_a.pdb "$BUILD_DIR/$target-$platform/lib$d/uv_a.pdb"
        fi;
      fi
    fi

    rm -f "$root/build/$(host_arch)-desktop/lib$d"/*.{so,la,dylib}*
    return
  fi

  if [ "$hosttarget" == "arm64" ]; then
    hosttarget="arm"
  fi

  # Use correct sdk, fixes:
  # ld: in /Users/ec2-user/app2/build/ios-simulator/lib/libuv.a(libuv_la-fs-poll.o), building for iOS Simulator, but linking in object file built for iOS, file '/Users/ec2-user/app2/build/ios-simulator/lib/libuv.a' for architecture arm64
  local sdk="iphoneos"
  [[ "$platform" == "iPhoneSimulator" ]] && sdk="iphonesimulator"

  export PLATFORM=$platform
  export CC="$(xcrun -sdk $sdk -find clang)"
  export CXX="$(xcrun -sdk $sdk -find clang++)"
  export STRIP="$(xcrun -sdk $sdk -find strip)"
  export LD="$(xcrun -sdk $sdk -find ld)"
  export CPP="$CC -E"
  export CFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -m$sdk-version-min=$SDKMINVERSION"
  export AR=$(xcrun -sdk $sdk -find ar)
  export RANLIB=$(xcrun -sdk $sdk -find ranlib)
  export CPPFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -m$sdk-version-min=$SDKMINVERSION"
  export LDFLAGS="-Wc,-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk"

  if ! test -f Makefile; then
    quiet ./configure --prefix="$BUILD_DIR/$target-$platform" --host="$hosttarget-apple-darwin"
  fi

  if [ ! $? = 0 ]; then
    echo "WARNING! - iOS will not be enabled. iPhone simulator not found, try \"sudo xcode-select --switch /Applications/Xcode.app\"."
    return
  fi

  quiet make "-j$CPU_CORES"
  quiet make install

  cd "$BUILD_DIR" || exit 1
  rm -f "$root/build/$target-$platform/lib$d"/*.{so,la,dylib}*
  echo "ok - built libuv for $target"
}

function _check_compiler_features {
  if [[ -n "$DEBUG" ]]; then
    return
  fi

  if [[ "$host" == "Win32" ]]; then
    # TODO(@mribbons) - https://github.com/socketsupply/socket/issues/150
    # Compiler test not working on windows, 9 unresolved externals
    return;
  fi

  echo "# checking compiler features"
  local cflags=($("$root/bin/cflags.sh"))
  local ldflags=($("$root/bin/ldflags.sh"))

  if [[ "$host" == "Darwin" ]]; then
    cflags+=(-x objective-c++)
  else
    cflags+=(-x c++)
  fi

  cflags+=("-I$root")

  $CXX "${cflags[@]}" "${ldflags[@]}" - -o /dev/null >/dev/null << EOF_CC
    #include "src/runtime.hh"
    int main () { return 0; }
EOF_CC

  die $? "not ok - $CXX ($("$CXX" -dumpversion)) failed in feature check required for building Socket Runtime"
}

function onsignal () {
  local status=${1:-$?}
  for pid in "${pids[@]}"; do
    kill TERM $pid >/dev/null 2>&1
    kill -9 "$pid" >/dev/null 2>&1
    wait "$pid" 2>/dev/null
  done
  exit "$status"
}

_prepare
cd "$BUILD_DIR" || exit 1

trap onsignal INT TERM

if [[ "$(uname -s)" == "Darwin" ]] && [[ -z "$NO_IOS" ]]; then
  quiet xcode-select -p
  die $? "not ok - xcode needs to be installed from the mac app store: https://apps.apple.com/us/app/xcode/id497799835"

  _compile_llama_metal arm64 iPhoneOS
  _compile_llama_metal arm64 iPhoneSimulator
  _compile_llama_metal x86_64 iPhoneSimulator
fi

{
  _compile_llama
  echo "ok - built libllama for desktop ($(host_arch))"
} & _compile_llama_pid=$!

# Although we're passing -j$CPU_CORES on non Win32, we still don't get max utiliztion on macos. Start this before fat libs.
{
  _compile_libuv
  echo "ok - built libuv for desktop ($(host_arch))"
} & _compile_libuv_pid=$!

if [[ "$(uname -s)" == "Darwin" ]] && [[ -z "$NO_IOS" ]]; then
  quiet xcode-select -p
  die $? "not ok - xcode needs to be installed from the mac app store: https://apps.apple.com/us/app/xcode/id497799835"

  SDKMINVERSION="13.0"
  export IPHONEOS_DEPLOYMENT_TARGET="13.0"

  LIPO=$(xcrun -sdk iphoneos -find lipo)
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"

  _setSDKVersion iPhoneOS

  _compile_libuv arm64 iPhoneOS & pids+=($!)
  _compile_llama arm64 iPhoneOS & pids+=($!)

  _compile_libuv x86_64 iPhoneSimulator & pids+=($!)
  _compile_llama x86_64 iPhoneSimulator & pids+=($!)

  if [[ "$arch" = "arm64" ]]; then
    _compile_libuv arm64 iPhoneSimulator & pids+=($!)
    _compile_llama arm64 iPhoneSimulator & pids+=($!)
  fi

  for pid in "${pids[@]}"; do wait "$pid"; done

  die $? "not ok - unable to combine build artifacts"
  echo "ok - created fat library"

  unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
    CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET

  die $? "not ok - could not copy fat library"
  echo "ok - copied fat library"
fi

if [[ "$host" != "Win32" ]]; then
  # non windows hosts uses make -j$CPU_CORES, wait for them to finish.
  # wait $_compile_libuv_pid
  wait $_compile_llama_pid
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  for abi in $(android_supported_abis); do
    _compile_libuv_android "$abi" & pids+=($!)
    _compile_llama "$abi" android & pids+=($!)
  done
fi

mkdir -p  "$SOCKET_HOME"/uv/{src/unix,include}
cp -fr "$BUILD_DIR"/uv/LICENSE "$SOCKET_HOME"/uv/LICENSE
cp -fr "$BUILD_DIR"/uv/src/*.{c,h} "$SOCKET_HOME"/uv/src
cp -fr "$BUILD_DIR"/uv/src/unix/*.{c,h} "$SOCKET_HOME"/uv/src/unix
die $? "not ok - could not copy headers"
echo "ok - copied headers"
cd "$CWD" || exit 1

cd "$BUILD_DIR" || exit 1

_get_web_view2

if [[ "$host" == "Win32" ]]; then
  # Wait for Win32 lib uv build
  # wait $_compile_libuv_pid
  wait $_compile_llama_pid
fi

_check_compiler_features
_build_runtime_library
_build_cli & pids+=($!)

_prebuild_desktop_main & pids+=($!)

echo "arch: $arch"

if [[ "$host" = "Darwin" ]] && [[ -z "$NO_IOS" ]]; then
  if test -d "$(xcrun -sdk iphoneos -show-sdk-path 2>/dev/null)"; then
    _prebuild_ios_main & pids+=($!)
    _prebuild_ios_simulator_main "x86_64" & pids+=($!)
    if [[ "$arch" = "arm64" ]]; then
      _prebuild_ios_simulator_main "arm64" iPhoneSimulator & pids+=($!)
    fi
  fi
fi

for pid in "${pids[@]}"; do
  wait "$pid" 2>/dev/null
  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"
done

_install "$(host_arch)" desktop

if [[ "$host" = "Darwin" ]] && [[ -z "$NO_IOS" ]]; then
  _install arm64 iPhoneOS
  _install x86_64 iPhoneSimulator

  if [[ "$arch" = "arm64" ]]; then
    _install arm64 iPhoneSimulator
  fi
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  for abi in $(android_supported_abis); do
    _install "$abi" android & pids+=($!)
  done
  wait
fi

_install_cli

exit $?
