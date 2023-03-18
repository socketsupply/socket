#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/functions.sh"
source "$root/bin/android-functions.sh"

if [[ -z "$CPU_CORES" ]]; then
  export CPU_CORES=$(set_cpu_cores)
fi

if [[ -n $VERBOSE ]]; then
  echo "# using cores: $CPU_CORES"
fi

declare args=()
declare pids=()
declare force=0
declare pass_force=""
declare host="$(host_os)"

LIPO=""
declare CWD=$(pwd)
declare PREFIX="${PREFIX:-"/usr/local"}"
declare BUILD_DIR="$CWD/build"

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
    pass_force="$arg"
    force=1; continue   
  fi

  args+=("$arg")
done

if [ -n "$LOCALAPPDATA" ] && [ -z "$SOCKET_HOME" ]; then
  SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
else
  SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"
fi

echo "SOCKET_HOME: $SOCKET_HOME"

if [ -z "$SOCKET_HOME" ]; then
  if [ -n "$LOCALAPPDATA" ]; then
    SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
  fi
fi

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
  # Libs are copied to build/platform/lib$d (uv_a.lib didn't support being renamed, this would require modification of the build chain)
  # libsocket-runtime.a is now named libsocket-runtime$d.a
  # ssc build --prod defines whether or not the app is being built for debug
  # and therefore links to the app being built to the correct libsocket-runtime$d.a
  if [[ ! -z "$DEBUG" ]]; then
    d="d"
  fi
fi

if [ ! "$CXX" ]; then
  if command -v clang++ >/dev/null 2>&1; then
    CXX="$(command -v clang++)"
  elif command -v g++ >/dev/null 2>&1; then
    CXX="$(command -v g++)"
  fi

  if [ "$host" = "Win32" ]; then
    if command -v $CXX >/dev/null 2>&1; then
      echo > /dev/null
    else
      # POSIX doesn't handle quoted commands
      # Quotes inside variables don't escape spaces, quotes only help on the line we are executing
      # Make a temp link
      CXX_TMP=$(mktemp)
      rm $CXX_TMP
      ln -s "$CXX" $CXX_TMP
      CXX=$CXX_TMP
      # Make tmp.etc look like clang++.etc, makes clang output look correct
      CXX=$(echo $CXX|sed 's/tmp\./clang++\./')
      mv $CXX_TMP $CXX

    fi
  fi
  
  echo Using $CXX as CXX

  if [ ! "$CXX" ]; then
    echo "not ok - could not determine \$CXX environment variable"
    exit 1
  fi
fi

export CXX

if [[ $host!="Win32" ]]; then
  if ! quiet command -v sudo; then
    sudo () {
      $@
      return $?
    }
  fi
fi

function advice {
  if [[ "$(uname -s)" == "Darwin" ]]; then
    echo "brew install $1"
  elif [[ "$(uname -r)" == *"ARCH"* ]]; then
    echo "sudo pacman -S $1"
  elif [[ "$(uname -s)" == *"Linux"* ]]; then
    echo "apt-get install $1"
  fi
}

if [[ "$(uname -s)" != *"_NT"* ]]; then
  quiet command -v make
  die $? "not ok - missing build tools, try \"$(advice "make")\""
fi

if [ "$host" == "Darwin" ]; then
  quiet command -v automake
  die $? "not ok - missing build tools, try \"$(advice "automake")\""
  quiet command -v glibtoolize
  die $? "not ok - missing build tools, try 'brew install libtool'"
  quiet command -v libtool
  die $? "not ok - missing build tools, try 'brew install libtool'"
fi

if [ "$host" == "Linux" ]; then
  quiet command -v autoconf
  die $? "not ok - missing build tools, try \"$(advice "autoconf")\""
  quiet command -v pkg-config
  die $? "not ok - missing pkg-config tool, \"$(advice 'pkg-config')\""
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  abis=($(android_supported_abis))
  platform="android"
  arch="${abis[0]}"
  host_arch="$(uname -m)"
  clang="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch" "$arch")"
  
  if ! quiet $clang -v; then
    echo "Android clang call failed. This could indicate an issue with ANDROID_HOME, missing ndk tools, or incorrectly determined host or target architectures."
    exit 1
  fi
fi

function _build_cli {
  echo "# building cli for desktop ($(uname -m))..."
  local arch="$(uname -m)"
  local platform="desktop"

  local src="$root/src"
  local output_directory="$BUILD_DIR/$arch-$platform"

  # Expansion won't work under _NT
  # uv found by -L
  # referenced directly below
  # local libs=($("echo" -l{socket-runtime}))
  local libs=""

  if [[ "$(uname -s)" != *"_NT"* ]]; then
    libs=($("echo" -l{uv,socket-runtime}))
  fi

  if [[ ! -z "$VERBOSE" ]]; then
    echo "cli libs: $libs, $(uname -s)"
  fi

  local ldflags=($("$root/bin/ldflags.sh" --arch "$arch" --platform "$platform" ${libs[@]}))
  local cflags=(-DSSC_CLI $("$root/bin/cflags.sh"))

  local test_sources=($(find "$src"/cli/*.cc 2>/dev/null))
  local sources=()
  local outputs=()

  mkdir -p "$BUILD_DIR/$arch-$platform/bin"

  for source in "${test_sources[@]}"; do
    local output="${source/$src/$output_directory}"
    # For some reason cli causes issues when debug and release are in the same folder
    output="${output/.cc/$d.o}"
    output="${output/cli/cli$d}"
    if (( force )) || ! test -f "$output" || (( $(stat_mtime "$source") > $(stat_mtime "$output") )); then
      sources+=("$source")
      outputs+=("$output")
    fi
  done

  for (( i = 0; i < ${#sources[@]}; i++ )); do
    mkdir -p "$(dirname "${outputs[$i]}")"
    quiet $CXX "${cflags[@]}"  \
      -c "${sources[$i]}"        \
      -o "${outputs[$i]}"
    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} -c \"${sources[$i]}\" -o \"${outputs[$i]}\""
  done

  local exe=""
  local libsocket_win=""
  local test_sources=($(find "$BUILD_DIR/$arch-$platform"/cli$d/*$d.o 2>/dev/null))
  if [[ "$(uname -s)" == *"_NT"* ]]; then
    declare d=""
    if [[ ! -z "$DEBUG" ]]; then
      d="d"
    fi
    exe=".exe"
    libsocket_win="$BUILD_DIR/$arch-$platform/lib$d/libsocket-runtime$d.a"
    test_sources+=("$libsocket_win")
  fi

  libs=($(find "$root/build/$arch-$platform/lib$d/*" 2>/dev/null))
  test_sources+=(${libs[@]})
  local build_ssc=0
  local ssc_output="$BUILD_DIR/$arch-$platform/bin/ssc$exe"

  for source in "${test_sources[@]}"; do
    if (( force )) || ! test -f "$ssc_output" || (( $(stat_mtime "$source") > $(stat_mtime "$ssc_output") )); then
      build_ssc=1
      # break
    fi
  done


  if (( build_ssc )); then
    quiet $CXX                                   \
      "$BUILD_DIR/$arch-$platform"/cli$d/*$d.o   \
      "${cflags[@]}" "${ldflags[@]}"             \
      "$libsocket_win" "$libwebview_win"         \
      -o "$ssc_output"

    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} \"${ldflags[@]}\" -o \"$BUILD_DIR/$arch-$platform/bin/ssc\""
    echo "ok - built the cli for desktop"
  fi
}

function _build_runtime_library {
  echo "# building runtime library"
  "$root/bin/build-runtime-library.sh" --arch "$(uname -m)" --platform desktop $pass_force & pids+=($!)
  if [[ "$host" = "Darwin" ]]; then
    "$root/bin/build-runtime-library.sh" --arch "$(uname -m)" --platform ios $pass_force & pids+=($!)
    "$root/bin/build-runtime-library.sh" --arch x86_64 --platform ios-simulator $pass_force & pids+=($!)
  fi

  if [[ -n "$BUILD_ANDROID" ]]; then
    for abi in $(android_supported_abis); do
      "$root/bin/build-runtime-library.sh" --platform android --arch "$abi" $pass_force & pids+=($!)
    done
  fi

  wait
}

function _get_web_view2() {
  if [[ "$(uname -s)" != *"_NT"* ]]; then
    return
  fi
  
  local arch="$(uname -m)"
  local platform="desktop"

  if test -f "$BUILD_DIR/$arch-$platform/lib$d/WebView2LoaderStatic.lib"; then
    echo "$BUILD_DIR/$arch-$platform/lib$d/WebView2LoaderStatic.lib exists."
    return
  fi

  local tmp=$(mktemp -d)
  local pwd=$(pwd)

  echo "# Downloading Webview2"

  curl -L https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.1619-prerelease --output $tmp/webview2.zip
  cd $tmp
  unzip -q $tmp/webview2.zip
  mkdir -p "$BUILD_DIR/include"
  mkdir -p "$BUILD_DIR/$arch-$platform/lib$d"/

  cp -pf build/native/include/WebView2.h "$BUILD_DIR/include/WebView2.h" 
  cp -pf build/native/include/WebView2Experimental.h "$BUILD_DIR/include/WebView2Experimental.h" 
  cp -pf build/native/include/WebView2EnvironmentOptions.h "$BUILD_DIR/include/WebView2EnvironmentOptions.h" 
  cp -pf build/native/include/WebView2ExperimentalEnvironmentOptions.h "$BUILD_DIR/include/WebView2ExperimentalEnvironmentOptions.h" 
  cp -pf build/native/x64/WebView2LoaderStatic.lib "$BUILD_DIR/$arch-$platform/lib$d/WebView2LoaderStatic.lib" 

  cd $pwd

  rm -rf $tmp
}

function _prebuild_desktop_main () {
  echo "# precompiling main program for desktop"
  local arch="$(uname -m)"
  local platform="desktop"

  local src="$root/src"
  local objects="$BUILD_DIR/$arch-$platform/objects"

  local cflags=($("$root/bin/cflags.sh"))
  local test_sources=($(find "$src"/desktop/*.{cc,mm} 2>/dev/null))
  local sources=()
  local outputs=()

  mkdir -p "$(dirname "$output")"

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
    quiet $CXX "${cflags[@]}" \
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
  local test_sources=($(find "$src"/ios/ios.mm 2>/dev/null))
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
  local arch="x86_64"
  local platform="iPhoneSimulator"

  local src="$root/src"
  local objects="$BUILD_DIR/$arch-$platform/objects"

  local clang="$(xcrun -sdk iphonesimulator -find clang++)"
  local cflags=($(TARGET_IPHONE_SIMULATOR=1 "$root/bin/cflags.sh"))
  local test_sources=($(find "$src"/ios/ios.mm 2>/dev/null))
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
  echo "ok - precompiled main program for iOS Simulator"
}

function _prepare {
  echo "# preparing directories..."
  rm -rf "$SOCKET_HOME"/{lib$d,src,bin,include,objects,api}
  rm -rf "$SOCKET_HOME"/{lib$d,objects}/"$(uname -m)-desktop"

  mkdir -p "$SOCKET_HOME"/{lib$d,src,bin,include,objects,api}
  mkdir -p "$SOCKET_HOME"/{lib$d,objects}/"$(uname -m)-desktop"

  if [[ "$host" = "Darwin" ]]; then
    mkdir -p "$SOCKET_HOME"/{lib$d,objects}/{arm64-iPhoneOS,x86_64-iPhoneSimulator}
  fi

  if [[ -n $BUILD_ANDROID ]]; then
    for abi in $(android_supported_abis); do
      mkdir -p "$SOCKET_HOME"/{lib$d,objects}/"$abi"
    done
  fi

  if [ ! -d "$BUILD_DIR/uv" ]; then
    git clone --depth=1 https://github.com/socketsupply/libuv.git $BUILD_DIR/uv > /dev/null 2>&1
    rm -rf $BUILD_DIR/uv/.git
    # Comment out compiler tests, we've covered these sufficiently for now
    if [[ -z "$ENABLE_LIBUV_C_COMPILER_CHECKS" ]]; then
      tempmkl=$(mktemp)
      sed 's/check_c_compiler_flag/# check_c_compiler_flag/' $BUILD_DIR/uv/CMakeLists.txt > $tempmkl
      mv $tempmkl $BUILD_DIR/uv/CMakeLists.txt
    fi

    die $? "not ok - unable to clone. See trouble shooting guide in the README.md file"
  fi

  echo "ok - directories prepared"
}

function _install {
  declare arch="$1"
  declare platform="$2"

  if [ $platform == "desktop" ]; then
    echo "# copying sources to $SOCKET_HOME/src"
    cp -r "$CWD"/src/* "$SOCKET_HOME/src"
  fi

  # TODO(@mribbons): Set lib types based on platform, after mobile CI is working

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

  if [[ $platform == "android" ]]; then
    # Debug builds not currently supported for android
    _d=""
  fi

  if test -d "$BUILD_DIR/$arch-$platform"/lib$_d; then
    echo "# copying libraries to $SOCKET_HOME/lib$_d/$arch-$platform"
    rm -rf "$SOCKET_HOME/lib$_d/$arch-$platform"
    mkdir -p "$SOCKET_HOME/lib$_d/$arch-$platform"
    if [[ $platform != "android" ]]; then
      cp -rfp "$BUILD_DIR/$arch-$platform"/lib$_d/*.a "$SOCKET_HOME/lib$_d/$arch-$platform"
    fi
    if [[ $host=="Win32" ]] && [[ $platform == "desktop" ]]; then
      cp -rfp "$BUILD_DIR/$arch-$platform"/lib$_d/*.lib "$SOCKET_HOME/lib$_d/$arch-$platform"
    fi

    if [[ $platform == "android" ]] && [[ -d "$BUILD_DIR/$arch-$platform"/lib ]]; then
      cp -fr "$BUILD_DIR/$arch-$platform"/lib/*.a "$SOCKET_HOME/lib/$arch-$platform"
    fi
  else 
     echo >&2 "not ok - Missing $BUILD_DIR/$arch-$platform/lib"
    exit 1
  fi

  if [ $platform == "desktop" ]; then
    echo "# copying js api to $SOCKET_HOME/api"
    mkdir -p "$SOCKET_HOME/api"
    cp -frp "$root"/api/* "$SOCKET_HOME/api"

    # only do this for desktop, no need to copy again for other platforms
    rm -rf "$SOCKET_HOME/include"
    mkdir -p "$SOCKET_HOME/include"
    cp -rfp "$BUILD_DIR"/uv/include/* $SOCKET_HOME/include
  fi

  if [[ "$(uname -s)" == *"_NT"* ]]; then
    if [ $platform == "desktop" ]; then
      mkdir -p "$SOCKET_HOME/ps1"
      cp -ap $root/bin/*.ps1 $SOCKET_HOME/ps1
      cp -ap $root/bin/.vs* $SOCKET_HOME/ps1
    fi
  fi

}

function _install_cli {
  local arch="$(uname -m)"

  if [ -z "$TEST" ] && [ -z "$NO_INSTALL" ]; then
    echo "# moving binary to '$SOCKET_HOME/bin' (prompting to copy file into directory)"

    cp -f "$BUILD_DIR/$arch-desktop"/bin/* "$SOCKET_HOME/bin"
    die $? "not ok - unable to move binary into '$SOCKET_HOME'"

    if [[ "$SOCKET_HOME" != "$PREFIX" ]]; then
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
  local host_arch="$(uname -m)"
  clang="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch" "$arch")"
  ar="$(android_ar "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch")"
  android_includes=$(android_arch_includes "$arch")

  local cflags=(-std=gnu89 -g -pedantic -I"$root"/build/uv/include -I"$root"/build/uv/src -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGEFILE_SOURCE -fPIC -Wall -Wextra -Wno-pedantic -Wno-sign-compare -Wno-unused-parameter -Wno-implicit-function-declaration)
  cflags+=("${android_includes[*]}")
  local objects=()
  local sources=("unix/async.c" "unix/core.c" "unix/dl.c" "unix/fs.c" "unix/getaddrinfo.c" "unix/getnameinfo.c" "unix/linux.c" "unix/loop.c" "unix/loop-watcher.c" "unix/pipe.c" "unix/poll.c" "unix/process.c" "unix/proctitle.c" "unix/random-devurandom.c" "unix/random-getentropy.c" "unix/random-getrandom.c" "unix/random-sysctl-linux.c" "unix/signal.c" "unix/stream.c" "unix/tcp.c" "unix/thread.c" "unix/tty.c" "unix/udp.c" fs-poll.c idna.c inet.c random.c strscpy.c strtok.c threadpool.c timer.c uv-common.c uv-data-getter-setters.c version.c)  

  declare output_directory="$root/build/$arch-$platform/uv$d"
  mkdir -p "$output_directory"

  declare src_directory="$root/build/uv/src"
  
  trap onsignal INT TERM
  local i=0
  local max_concurrency=$CPU_CORES
  build_static=0
  declare base_lib="libuv"
  declare static_library="$root/build/$arch-$platform/lib$d/$base_lib$d.a"

  for source in "${sources[@]}"; do
    if (( i++ > max_concurrency )); then
      for pid in "${pids[@]}"; do
        wait "$pid" 2>/dev/null
      done
      i=0
    fi

    declare object="${source/.c/$d.o}"
    object=$(basename $object)
    objects+=("$output_directory/$object")

    {
      if (( force )) || ! test -f "$output_directory/$object" || (( $(stat_mtime "$src_directory/$source") > $(stat_mtime "$output_directory/$object") )); then
        mkdir -p "$(dirname "$object")"
        echo "# compiling object ($arch-$platform) $(basename "$source")"
        quiet $clang "${cflags[@]}" -c "$src_directory/$source" -o "$output_directory/$object" || onsignal
        echo "ok - built $source -> $object ($arch-$platform)"
        # Can't write back to variable in block, remove final library to force rebuild
        rm $static_library 2>/dev/null
      fi
    } & pids+=($!)
  done

  for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null
  done

  if [ ! -f $static_library ]; then
    build_static=1
  fi
  mkdir -p "$(dirname $static_library)"

  if (( build_static )); then
    quiet $ar crs "$static_library" "${objects[@]}"
    if [ -f $static_library ]; then
      echo "ok - built $base_lib ($arch-$platform): $(basename "$static_library")"
    else
       echo >&2 "not ok - failed to build $static_library"
      exit 1
    fi
  
  else
    if [ -f $static_library ]; then
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
  lib_size=$(stat_size $static_library)
  if (( lib_size < $(android_min_expected_static_lib_size "$base_lib") )); then
    echo >&2 "not ok - $static_library size looks wrong: $lib_size, renaming as .bad"
    mv $static_library $static_library.bad
    exit 1
  fi
}

function _compile_libuv {
  target=$1
  hosttarget=$1
  platform=$2

  if [ -z "$target" ]; then
    target=$(uname -m)
    platform="desktop"
  fi

  echo "# building libuv for $platform ($target) on $host..."
  STAGING_DIR=$BUILD_DIR/$target-$platform/uv

  if [ ! -d "$STAGING_DIR" ]; then
    mkdir -p "$STAGING_DIR"
    cp -r $BUILD_DIR/uv/* $STAGING_DIR
    cd $STAGING_DIR
    # Doesn't work in mingw
    if [[ "$host" != "Win32" ]]; then
      quiet sh autogen.sh
    fi;
  else
    cd $STAGING_DIR
  fi

  mkdir -p "$STAGING_DIR/build/"

  if [ $platform == "desktop" ]; then
    if [[ "$host" != "Win32" ]]; then
      if ! test -f Makefile; then
        quiet ./configure --prefix=$BUILD_DIR/$target-$platform
        die $? "not ok - desktop configure"

        quiet make clean
        quiet make -j$CPU_CORES
        quiet make install
      fi
    else
      if ! test -f "$BUILD_DIR/$target-$platform/lib$d/uv_a.lib"; then
        local config="Release"
        if [[ ! -z "$DEBUG" ]]; then
          config="Debug"
        fi
        cd "$STAGING_DIR/build/"
        quiet cmake .. -DBUILD_TESTING=OFF -DLIBUV_BUILD_SHARED=OFF
        cd $STAGING_DIR
        quiet cmake --build $STAGING_DIR/build/ --config $config
        mkdir -p $BUILD_DIR/$target-$platform/lib$d
        quiet echo "cp -up $STAGING_DIR/build/$config/uv_a.lib $BUILD_DIR/$target-$platform/lib$d/uv_a.lib"
        cp -up $STAGING_DIR/build/$config/uv_a.lib $BUILD_DIR/$target-$platform/lib$d/uv_a.lib
        if [[ ! -z "$DEBUG" ]]; then
          cp -up $STAGING_DIR/build/$config/uv_a.pdb $BUILD_DIR/$target-$platform/lib$d/uv_a.pdb
        fi;
      fi
    fi

    rm -f "$root/build/$(uname -m)-desktop/lib$d"/*.{so,la,dylib}*
    return
  fi

  if [ $hosttarget == "arm64" ]; then
    hosttarget="arm"
  fi

  export PLATFORM=$platform
  export CC="$(xcrun -sdk iphoneos -find clang)"
  export STRIP="$(xcrun -sdk iphoneos -find strip)"
  export LD="$(xcrun -sdk iphoneos -find ld)"
  export CPP="$CC -E"
  export CFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -miphoneos-version-min=$SDKMINVERSION"
  export AR=$(xcrun -sdk iphoneos -find ar)
  export RANLIB=$(xcrun -sdk iphoneos -find ranlib)
  export CPPFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -miphoneos-version-min=$SDKMINVERSION"
  export LDFLAGS="-Wc,-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk"

  if ! test -f Makefile; then
    quiet ./configure --prefix=$BUILD_DIR/$target-$platform --host=$hosttarget-apple-darwin
  fi

  if [ ! $? = 0 ]; then
    echo "WARNING! - iOS will not be enabled. iPhone simulator not found, try \"sudo xcode-select --switch /Applications/Xcode.app\"."
    return
  fi

  quiet make -j$CPU_CORES
  quiet make install

  cd $BUILD_DIR
  rm -f "$root/build/$target-$platform/lib$d"/*.{so,la,dylib}*
  echo "ok - built for $target"
}

function _check_compiler_features {
  if [[ $host=="Win32" ]]; then
    # TODO(@mribbons) - https://github.com/socketsupply/socket/issues/150
    # Compiler test not working on windows, 9 unresolved externals
    return;
  fi

  echo "# checking compiler features"
  local cflags=($("$root/bin/cflags.sh"))
  if [[ -z "$DEBUG" ]]; then
    cflags+=(-o /dev/null)
  fi

  $CXX -x c++ "${cflags[@]}" - -o /dev/null >/dev/null << EOF_CC
    #include "src/common.hh"
    int main () { return 0; }
EOF_CC

  die $? "not ok - $CXX ($(\"$CXX\" -dumpversion)) failed in feature check required for building Socket Rutime"
}

function onsignal () {
  local status=${1:-$?}
  for pid in "${pids[@]}"; do
    kill TERM $pid >/dev/null 2>&1
    kill -9 $pid >/dev/null 2>&1
    wait "$pid" 2>/dev/null
  done
  exit $status
}

_check_compiler_features
_prepare
cd $BUILD_DIR

trap onsignal INT TERM

# Although we're passing -j$CPU_CORES on non Win32, we still don't get max utiliztion on macos. Start this before fat libs.
{
  _compile_libuv
  echo "ok - built libuv for $platform ($target)"
} & _compile_libuv_pid=$!

if [[ "$(uname -s)" == "Darwin" ]]; then
  quiet xcode-select -p
  die $? "not ok - xcode needs to be installed from the mac app store: https://apps.apple.com/us/app/xcode/id497799835"

  SDKMINVERSION="8.0"
  export IPHONEOS_DEPLOYMENT_TARGET="8.0"

  LIPO=$(xcrun -sdk iphoneos -find lipo)
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

  _setSDKVersion iPhoneOS

  _compile_libuv arm64 iPhoneOS & pids+=($!)
  _compile_libuv x86_64 iPhoneSimulator & pids+=($!)

  for pid in "${pids[@]}"; do wait $pid; done

  die $? "not ok - unable to combine build artifacts"
  echo "ok - created fat library"

  unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
    CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET

  die $? "not ok - could not copy fat library"
  echo "ok - copied fat library"
fi

if [[ "$host" != "Win32" ]]; then
  # non windows hosts uses make -j$CPU_CORES, wait for them to finish.
  wait $_compile_libuv_pid
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  for abi in $(android_supported_abis); do
    _compile_libuv_android "$abi" & pids+=($!)
  done
fi

mkdir -p  $SOCKET_HOME/uv/{src/unix,include}
cp -fr $BUILD_DIR/uv/src/*.{c,h} $SOCKET_HOME/uv/src
cp -fr $BUILD_DIR/uv/src/unix/*.{c,h} $SOCKET_HOME/uv/src/unix
die $? "not ok - could not copy headers"
echo "ok - copied headers"
cd $CWD

cd "$BUILD_DIR"

_get_web_view2

if [[ "$HOST" == "Win32" ]]; then
  # Wait for Win32 lib uv build
  wait $_compile_libuv_pid
fi

_build_runtime_library
_build_cli & pids+=($!)

_prebuild_desktop_main & pids+=($!)

if [[ "$host" = "Darwin" ]]; then
  if test -d "$(xcrun -sdk iphoneos -show-sdk-path 2>/dev/null)"; then
    _prebuild_ios_main & pids+=($!)
    _prebuild_ios_simulator_main & pids+=($!)
  fi
fi

for pid in "${pids[@]}"; do
  wait "$pid" 2>/dev/null
  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"
done

_install "$(uname -m)" desktop

if [[ "$host" = "Darwin" ]]; then
  _install arm64 iPhoneOS
  _install x86_64 iPhoneSimulator
fi

if [[ -n "$BUILD_ANDROID" ]]; then
  for abi in $(android_supported_abis); do
    _install "$abi" android & pids+=($!)
  done
  wait
fi

_install_cli

exit $?