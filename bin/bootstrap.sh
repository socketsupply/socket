#!/usr/bin/env bash
set -e;

PREFIX=${PREFIX:-$HOME}
PLATFORMPATH=""
ASSETS_DIR=""
SDKVERSION=""
LIPO=""

if [ ! "$CXX" ]; then
  if [ ! -z "$LOCALAPPDATA" ]; then
    if which clang++ >/dev/null 2>&1; then
      CXX="$(which clang++)"
    fi
  fi

  if [ ! "$CXX" ]; then
    if which g++ >/dev/null 2>&1; then
      CXX="$(which g++)"
    elif which clang++ >/dev/null 2>&1; then
      CXX="$(which clang++)"
    fi
  fi

  if [ ! "$CXX" ]; then
    echo "• Error: Could not determine \$CXX environment variable"
    exit 1
  else
    echo "• Warning: \$CXX environment variable not set, assuming '$CXX'"
  fi
fi

if ! which sudo > /dev/null 2>&1; then
  sudo () {
    $@
    return $?
  }
fi

function _build {
  echo '• Building ssc'

  "$CXX" src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a \
    -DVERSION_HASH=`git rev-parse --short HEAD` \
    -DVERSION=`cat VERSION.txt` \

  if [ ! $? = 0 ]; then
    echo "• Unable to build. See trouble shooting guide in the README.md file"
    exit 1
  fi
  echo "• Success"
}

function _prepare {
  if [ ! -z "$LOCALAPPDATA" ]; then
    ASSETS_DIR="$LOCALAPPDATA/Programs/socketsupply"
  else
    ASSETS_DIR="$PREFIX/.config/socket-sdk"
  fi

  echo "• Preparing directories"
  rm -rf "$ASSETS_DIR"
  mkdir -p $ASSETS_DIR/{lib,src,include}
}

function _install {
  cp -r `pwd`/src "$ASSETS_DIR"

  echo "• Copying sources to $ASSETS_DIR/src"

  if [ -d `pwd`/lib ]; then
    echo "• Copying libraries to $ASSETS_DIR/lib"
    mkdir -p "$ASSETS_DIR/lib"
    cp -r `pwd`/lib/* "$ASSETS_DIR/lib"
  fi

  echo "• Moving binary to $PREFIX/bin (prompting to copy file into directory)"
  sudo mv `pwd`/bin/cli "/usr/local/bin/ssc"

  if [ ! $? = 0 ]; then
    echo "• Unable to move binary into place"
    exit 1
  fi

  echo -e '• Finished. Type "ssc -h" for help'
  exit 0
}

#
# Re-compile libudx for iOS (and the iOS simulator).
#
function _setSDKVersion {
  sdks=`ls $PLATFORMPATH/$1.platform/Developer/SDKs`
  arr=()
  for sdk in $sdks
  do
   echo $sdk
   arr[${#arr[@]}]=$sdk
  done

  # Last item will be the current SDK, since it is alpha ordered
  count=${#arr[@]}
  if [ $count -gt 0 ]; then
   sdk=${arr[$count-1]:${#1}}
   num=`expr ${#sdk}-4`
   SDKVERSION=${sdk:0:$num}
  else
   SDKVERSION="8.0"
  fi
}

function _compile_libuv {
  target=$1
  hosttarget=$1
  platform=$2

  if [[ $hosttarget == "x86_64" ]]; then
    xxhosttarget="i386"
  elif [[ $hosttarget == "arm64" ]]; then
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

  ./configure --prefix="$BUILD_DIR/output/$target" --host=$hosttarget-apple-darwin

  make clean
  make -j 10
  make install
  install_name_tool -id libuv.1.dylib $BUILD_DIR/output/$target/lib/libuv.1.dylib
}

function _cross_compile_libuv {
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

  export IPHONEOS_DEPLOYMENT_TARGET="8.0"
  OLD_CWD=`pwd`
  BUILD_DIR=`pwd`/build
  LIB_DIR=`pwd`/lib
  rm -rf $LIB_DIR/uv

  #
  # Shallow clone the main branch of libuv
  #
  if [ ! -d "$BUILD_DIR" ]; then
  	git clone --depth=1 git@github.com:libuv/libuv.git build
  fi

  cd $BUILD_DIR
  sh autogen.sh

  #
  # Build artifacts for all platforms
  #
  _compile_libuv arm64 iPhoneOS
  _compile_libuv i386 iPhoneSimulator
  _compile_libuv x86_64 iPhoneSimulator

  #
  # Combine the build artifacts
  #

  $LIPO -create \
    $BUILD_DIR/output/arm64/lib/libuv.a \
    $BUILD_DIR/output/x86_64/lib/libuv.a \
    $BUILD_DIR/output/i386/lib/libuv.a \
    -output libuv.a
  $LIPO -create \
    $BUILD_DIR/output/arm64/lib/libuv.1.dylib \
    $BUILD_DIR/output/x86_64/lib/libuv.1.dylib \
    $BUILD_DIR/output/i386/lib/libuv.1.dylib \
    -output libuv.1.dylib

  install_name_tool -id @rpath/libuv.1.dylib libuv.1.dylib

  $LIPO -info libuv.a

  #
  # Copy the build into the project and delete leftover build artifacts.
  #
  mkdir -p $LIB_DIR/uv/include >/dev/null 2>&1;

  cp libuv.a $LIB_DIR
  cp -r $BUILD_DIR/include $ASSETS_DIR

  cd $OLD_CWD

  unset PLATFORM
  unset CC
  unset STRIP
  unset LD
  unset CPP
  unset CFLAGS
  unset AR
  unset RANLIB
  unset CPPFLAGS
  unset LDFLAGS
  unset IPHONEOS_DEPLOYMENT_TARGET
}

#
# Build and Install
#
_prepare

if [ "$1" == "ios" ]; then
  #
  # Attempts to find iphoneos tool, will fail fast if xcode not installed
  #
  xcode-select -p >/dev/null 2>&1;
  if [ ! $? = 0 ]; then
    echo "Xcode needs to be installed from the Mac App Store."
    exit 1
  fi

  which autoconf >/dev/null 2>&1;
  if [ ! $? = 0 ]; then
    echo "Try 'brew install automake'"
    exit 1
  fi

  LIPO=$(xcrun -sdk iphoneos -find lipo)

  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  _setSDKVersion iPhoneOS
  SDKMINVERSION="8.0"

  _cross_compile_libuv
fi

_build
_install
