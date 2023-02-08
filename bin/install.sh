#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare pids=()

LIPO=""
declare CWD=$(pwd)
declare PREFIX="${PREFIX:-"/usr/local"}"
declare BUILD_DIR="$CWD/build"
declare SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"
declare host="$(uname -s)"

if [[ "$host" = "Linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
  echo "WSL is not supported."
  exit 1
  fi
elif [[ "$host" == *"MINGW64_NT"* ]]; then
  host="Win32"
elif [[ "$host" == *"MSYS_NT"* ]]; then
  echo "MSYS is not supported."
  exit 1
fi

if [ -n "$LOCALAPPDATA" ]; then
  SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
fi

if [ ! "$CXX" ]; then
  if command -v clang++ >/dev/null 2>&1; then
    CXX="$(command -v clang++)"
  elif command -v g++ >/dev/null 2>&1; then
    CXX="$(command -v g++)"
  fi

  if [ ! "$CXX" ]; then
    echo "not ok - could not determine \$CXX environment variable"
    exit 1
  fi
fi

export CXX

function quiet () {
  if [ -n "$VERBOSE" ]; then
    echo "$@"
    "$@"
  else
    "$@" > /dev/null 2>&1
  fi

  return $?
}

if ! quiet command -v sudo; then
  sudo () {
    $@
    return $?
  }
fi

function die {
  local status=$1
  if (( status != 0 && status != 127 )); then
    for pid in "${pids[@]}"; do
      kill TERM $pid >/dev/null 2>&1
      kill -9 $pid >/dev/null 2>&1
      wait "$pid" 2>/dev/null
    done
    echo "$2 - please report (https://discord.gg/YPV32gKCsH)"
    exit 1
  fi
}

function advice {
  if [[ "$(uname -s)" == "Darwin" ]]; then
    echo "brew install $1"
  elif [[ "$(uname -r)" == *"ARCH"* ]]; then
    echo "sudo pacman -S $1"
  elif [[ "$(uname -s)" == *"Linux"* ]]; then
    echo "apt-get install $1"
  elif [[ "$(uname -s)" == *"_NT"* ]]; then
    echo "choco install $1 ?"
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

  local ldflags=($("$root/bin/ldflags.sh" --arch "$arch" --platform "$platform" ${libs[@]}))
  local cflags=(-DCLI $("$root/bin/cflags.sh"))

  local sources=($(find "$src"/cli/*.cc 2>/dev/null))
  local outputs=()

  mkdir -p "$BUILD_DIR/$arch-$platform/bin"

  for source in "${sources[@]}"; do
    local output="${source/$src/$output_directory}"
    output="${output/.cc/.o}"
    outputs+=("$output")
  done

  for (( i = 0; i < ${#sources[@]}; i++ )); do
    mkdir -p "$(dirname "${outputs[$i]}")"
    quiet "$CXX" "${cflags[@]}"  \
      -c "${sources[$i]}"        \
      -o "${outputs[$i]}"
    die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} -c \"${sources[$i]}\" -o \"${outputs[$i]}\""
  done

  local exe=""
  local libsocket_win=""
  if [[ "$(uname -s)" == *"_NT"* ]]; then
    exe=".exe"
    libsocket_win="$BUILD_DIR/$arch-$platform/lib/libsocket-runtime.a"
  fi

  quiet "$CXX"                                 \
    "$BUILD_DIR/$arch-$platform"/cli/*.o       \
    "${cflags[@]}" "${ldflags[@]}"             \
    "$libsocket_win"                           \
    -o "$BUILD_DIR/$arch-$platform/bin/ssc$exe"

  die $? "not ok - unable to build. See trouble shooting guide in the README.md file:\n$CXX ${cflags[@]} \"${ldflags[@]}\" -o \"$BUILD_DIR/$arch-$platform/bin/ssc\""
  echo "ok - built the cli for desktop"
}

function _build_runtime_library {
  echo "# building runtime library"
  echo "$root/bin/build-runtime-library.sh" --arch "$(uname -m)" --platform desktop
  "$root/bin/build-runtime-library.sh" --arch "$(uname -m)" --platform desktop & pids+=($!)
  if [[ "$host" = "Darwin" ]]; then
    "$root/bin/build-runtime-library.sh" --arch "$(uname -m)" --platform ios & pids+=($!)
    "$root/bin/build-runtime-library.sh" --arch x86_64 --platform ios-simulator & pids+=($!)
  fi

  wait
}

function _get_web_view2() {
  if [[ "$(uname -s)" != *"_NT"* ]]; then
    return
  fi
  
  local arch="$(uname -m)"
  local platform="desktop"

  if test -f "$BUILD_DIR/$arch-$platform/lib/WebView2LoaderStatic.lib"; then
    echo "$BUILD_DIR/lib/$arch-$platform/WebView2LoaderStatic.lib exists."
    return
  fi

  local tmp=$(mktemp -d)
  local pwd=$(pwd)

  echo "# Downloading Webview2"

  curl https://globalcdn.nuget.org/packages/microsoft.web.webview2.1.0.1369-prerelease.nupkg --output $tmp/webview2.zip
  cd $tmp
  unzip -q $tmp/webview2.zip
  mkdir -p "$BUILD_DIR/include"
  mkdir -p "$BUILD_DIR/$arch-$platform/lib"/

  cp -pf build/native/include/WebView2.h "$BUILD_DIR/include/WebView2.h" 
  cp -pf build/native/include/WebView2Experimental.h "$BUILD_DIR/include/WebView2Experimental.h" 
  cp -pf build/native/include/WebView2EnvironmentOptions.h "$BUILD_DIR/include/WebView2EnvironmentOptions.h" 
  cp -pf build/native/include/WebView2ExperimentalEnvironmentOptions.h "$BUILD_DIR/include/WebView2ExperimentalEnvironmentOptions.h" 
  cp -pf build/native/x64/WebView2LoaderStatic.lib "$BUILD_DIR/$arch-$platform/lib/WebView2LoaderStatic.lib" 

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
  local sources=($(find "$src"/desktop/*.{cc,mm} 2>/dev/null))
  local outputs=()

  mkdir -p "$(dirname "$output")"

  for source in "${sources[@]}"; do
    local output="${source/$src/$objects}"
    output="${output/.cc/.o}"
    output="${output/.mm/.o}"
    outputs+=("$output")
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
  local sources=($(find "$src"/ios/ios.mm 2>/dev/null))
  local outputs=()

  mkdir -p "$objects"

  for source in "${sources[@]}"; do
    local output="${source/$src/$objects}"
    output="${output/.cc/.o}"
    output="${output/.mm/.o}"
    outputs+=("$output")
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
  local sources=($(find "$src"/ios/ios.mm 2>/dev/null))
  local outputs=()

  mkdir -p "$objects"

  for source in "${sources[@]}"; do
    local output="${source/$src/$objects}"
    output="${output/.cc/.o}"
    output="${output/.mm/.o}"
    outputs+=("$output")
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
  rm -rf "$SOCKET_HOME"

  mkdir -p "$SOCKET_HOME"/{lib,src,bin,include,objects,api}
  mkdir -p "$SOCKET_HOME"/{lib,objects}/"$(uname -m)-desktop"

  if [[ "$host" = "Darwin" ]]; then
    mkdir -p "$SOCKET_HOME"/{lib,objects}/{arm64-iPhoneOS,x86_64-iPhoneSimulator}
  fi

  if [ ! -d "$BUILD_DIR/uv" ]; then
  	git clone --depth=1 https://github.com/socketsupply/libuv.git $BUILD_DIR/uv > /dev/null 2>&1
    rm -rf $BUILD_DIR/uv/.git

    die $? "not ok - unable to clone. See trouble shooting guide in the README.md file"
  fi

  echo "ok - directories prepared"
}

function _install {
  declare arch="$1"
  declare platform="$2"
  echo "# copying sources to $SOCKET_HOME/src"
  cp -r "$CWD"/src/* "$SOCKET_HOME/src"

  if test -d "$BUILD_DIR/$arch-$platform/objects"; then
    echo "# copying objects to $SOCKET_HOME/objects/$arch-$platform"
    rm -rf "$SOCKET_HOME/objects/$arch-$platform"
    mkdir -p "$SOCKET_HOME/objects/$arch-$platform"
    cp -r "$BUILD_DIR/$arch-$platform/objects"/* "$SOCKET_HOME/objects/$arch-$platform"
  fi

  if test -d "$BUILD_DIR"/lib; then
    echo "# copying libraries to $SOCKET_HOME/lib"
    mkdir -p "$SOCKET_HOME/lib"
    cp -fr "$BUILD_DIR"/lib/*.a "$SOCKET_HOME/lib/"
  fi

  if test -d "$BUILD_DIR/$arch-$platform"/lib; then
    echo "# copying libraries to $SOCKET_HOME/lib/$arch-$platform"
    rm -rf "$SOCKET_HOME/lib/$arch-$platform"
    mkdir -p "$SOCKET_HOME/lib/$arch-$platform"
    cp -fr "$BUILD_DIR/$arch-$platform"/lib/*.a "$SOCKET_HOME/lib/$arch-$platform"
    cp -fr "$BUILD_DIR/$arch-$platform"/lib/*.lib "$SOCKET_HOME/lib/$arch-$platform"
  fi

  echo "# copying js api to $SOCKET_HOME/api"
  mkdir -p "$SOCKET_HOME/api"
  cp -fr "$root"/api/* "$SOCKET_HOME/api"
  rm -f "$SOCKET_HOME/api/importmap.json"
  "$root/bin/generate-api-import-map.sh" > "$SOCKET_HOME/api/importmap.json"

  rm -rf "$SOCKET_HOME/include"
  mkdir -p "$SOCKET_HOME/include"
  #cp -rf "$CWD"/include/* $SOCKET_HOME/include
  cp -rf "$BUILD_DIR"/uv/include/* $SOCKET_HOME/include
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
        echo "warn - Failed to link binrary to '$PREFIX/bin': Trying 'sudo'"
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

function _compile_libuv {
  target=$1
  hosttarget=$1
  platform=$2

  if [ -z "$target" ]; then
    target=$(uname -m)
    host=$(uname -s)
    platform="desktop"

    if [[ "$host" = "Linux" ]]; then
      if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
        host="Win32"
      fi
    elif [[ "$host" == *"MINGW64_NT"* ]]; then
      host="Win32"
    elif [[ "$host" == *"MSYS_NT"* ]]; then
      die $? "MSYS not supported."
    fi
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
        quiet make -j8
        quiet make install
      fi
    else
      local config="Release"
      if [[ ! -z "$DEBUG" ]]; then
        config="Debug"
      fi
      cd "$STAGING_DIR/build/"
      cmake .. -DBUILD_TESTING=OFF
      cd $STAGING_DIR
      cmake --build $STAGING_DIR/build/ --config $config
      mkdir -p $BUILD_DIR/$target-$platform/lib
      echo "cp -up $STAGING_DIR/build/config/*.lib $BUILD_DIR/$target-$platform/lib"
      cp -up $STAGING_DIR/build/$config/*.lib $BUILD_DIR/$target-$platform/lib
      if [ $config == "Debug" ]; then
        cp -up $STAGING_DIR/build/$config/*.pdb $BUILD_DIR/$target-$platform/lib
      fi
    fi

    rm -f "$root/build/$(uname -m)-desktop/lib"/*.{so,la,dylib}*
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

  quiet make -j8
  quiet make install

  cd $BUILD_DIR
  rm -f "$root/build/$target-$platform/lib"/*.{so,la,dylib}*
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

  $CXX -x c++ "${cflags[@]}" - >/dev/null << EOF_CC
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

_compile_libuv
echo "ok - built libuv for $platform ($target)"

mkdir -p  $SOCKET_HOME/uv/{src/unix,include}
cp -fr $BUILD_DIR/uv/src/*.{c,h} $SOCKET_HOME/uv/src
cp -fr $BUILD_DIR/uv/src/unix/*.{c,h} $SOCKET_HOME/uv/src/unix
die $? "not ok - could not copy headers"
echo "ok - copied headers"
cd $CWD

cd "$BUILD_DIR"

_get_web_view2 & pids+=($!)

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

_install_cli
