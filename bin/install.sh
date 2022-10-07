#!/usr/bin/env bash

declare dirname="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$dirname/generate-runtime-preload-sources.sh"

LIPO=""
WORK_DIR=`pwd`
PREFIX="${PREFIX:-$HOME}"
LIB_DIR=$WORK_DIR/lib
BUILD_DIR=$WORK_DIR/build

if [ ! "$CXX" ]; then
  if [ ! -z "$LOCALAPPDATA" ]; then
    if command -v clang++ >/dev/null 2>&1; then
      CXX="$(command -v clang++)"
    fi
  fi

  if [ ! "$CXX" ]; then
    if command -v g++ >/dev/null 2>&1; then
      CXX="$(command -v g++)"
    elif command -v clang++ >/dev/null 2>&1; then
      CXX="$(command -v clang++)"
    fi
  fi

  if [ ! "$CXX" ]; then
    echo "not ok - could not determine \$CXX environment variable"
    exit 1
  fi
fi

function quiet () {
  #"$@"
  "$@" > /dev/null 2>&1
}

if ! quiet command -v sudo; then
  sudo () {
    $@
    return $?
  }
fi

function die {
  if [ ! $1 = 0 ]; then
    echo "$2 - please report (https://discord.gg/YPV32gKCsH)" && exit 1
  fi
}

function advice {
  if [[ "`uname -s`" == "Darwin" ]]; then
    echo "brew install $1"
  elif [[ "`uname -r`" == *"ARCH"* ]]; then
    echo "sudo pacman -S $1"
  elif [[ "`uname -s`" == *"Linux"* ]]; then
    echo "apt-get install $1"
  fi
}

quiet command -v make
die $? "not ok - missing build tools, try \"$(advice "make")\""

if [ "$(uname)" == "Darwin" ]; then
  quiet command -v automake
  die $? "not ok - missing build tools, try \"$(advice "automake")\""
  quiet command -v glibtoolize
  die $? "not ok - missing build tools, try 'brew install libtool'"
  quiet command -v libtool
  die $? "not ok - missing build tools, try 'brew install libtool'"
fi

if [ "$(uname)" == "Linux" ]; then
  quiet command -v autoconf
  die $? "not ok - missing build tools, try \"$(advice "autoconf")\""
  quiet command -v pkg-config
  die $? "not ok - missing pkg-config tool, \"$(advice 'pkg-config')\""
fi

function _build {
  echo "# building preload JavaScript"
  generate-runtime-preload-sources
  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"

  echo "# building cli for desktop (`uname -m`)..."
  "$CXX" src/cli/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a \
    -DSSC_BUILD_TIME="$(date '+%s')" \
    -DSSC_VERSION_HASH=`git rev-parse --short HEAD` \
    -DSSC_VERSION=`cat VERSION.txt`

  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"
  echo "ok - built the cli for desktop"
}

function _prepare {
  if [ ! -z "$LOCALAPPDATA" ]; then
    ASSETS_DIR="$LOCALAPPDATA/Programs/socketsupply"
  else
    ASSETS_DIR="$PREFIX/.config/socket-sdk"
  fi

  echo "# preparing directories..."
  rm -rf "$ASSETS_DIR"
  mkdir -p $ASSETS_DIR/{lib,src,include}
  mkdir -p $LIB_DIR

  if [ ! -d "$BUILD_DIR/input" ]; then
  	git clone --depth=1 https://github.com/libuv/libuv.git $BUILD_DIR/input > /dev/null 2>&1
    rm -rf $BUILD_DIR/input/.git

    die $? "not ok - unable to clone. See trouble shooting guide in the README.md file"
  fi

  echo "ok - directories prepared."
}

function _install {
  cp -r `pwd`/src "$ASSETS_DIR"

  echo "# copying sources to $ASSETS_DIR/src"

  if [ -d `pwd`/lib ]; then
    echo "# copying libraries to $ASSETS_DIR/lib"
    mkdir -p "$ASSETS_DIR/lib"
    cp -r `pwd`/lib/* "$ASSETS_DIR/lib"
  fi

  if [ -z "$TEST" ]; then
    local binDest="/usr/local/bin/ssc"
    echo "# moving binary to $binDest (prompting to copy file into directory)"
    sudo mkdir -p /usr/local/bin
    sudo mv `pwd`/bin/cli $binDest
  fi

  die $? "not ok - unable to move binary into place"
  echo "ok - done. type 'ssc -h' for help"
  exit 0
}

function _setSDKVersion {
  sdks=`ls $PLATFORMPATH/$1.platform/Developer/SDKs`
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

  if [ -z "$target" ]; then
    target=`uname -m`
    platform="desktop"
  fi

  echo "# building libuv for $platform ($target)..."
  STAGING_DIR=$BUILD_DIR/$target-$platform

  if [ ! -d "$STAGING_DIR" ]; then
    cp -r $BUILD_DIR/input $STAGING_DIR
    cd $STAGING_DIR
 	  quiet sh autogen.sh
  else
    cd $STAGING_DIR
  fi

  if [ $platform == "desktop" ]; then
    mkdir -p $PREFIX
    quiet ./configure --prefix=$STAGING_DIR/build
    die $? "not ok - desktop configure"

    quiet make clean
    quiet make -j8
    quiet make install
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

  quiet ./configure --prefix=$STAGING_DIR/build --host=$hosttarget-apple-darwin

  if [ ! $? = 0 ]; then
    echo "WARNING! - iOS will not be enabled. iPhone simulator not found, try \"sudo xcode-select --switch /Applications/Xcode.app\"."
    return
  fi

  quiet make -j8
  quiet make install

  cd $BUILD_DIR
  echo "ok - built for $target"
}

_prepare
cd $BUILD_DIR

if [[ "`uname -s`" == "Darwin" ]]; then
  quiet xcode-select -p
  die $? "not ok - xcode needs to be installed from the mac app store: https://apps.apple.com/us/app/xcode/id497799835"

  SDKMINVERSION="8.0"
  export IPHONEOS_DEPLOYMENT_TARGET="8.0"

  LIPO=$(xcrun -sdk iphoneos -find lipo)
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

  _setSDKVersion iPhoneOS

  pids=""
  _compile_libuv arm64 iPhoneOS & pids="$pids $!"
  _compile_libuv x86_64 iPhoneSimulator & pids="$pids $!"

  for pid in $pids; do wait $pid; done

  quiet $LIPO -create \
    $BUILD_DIR/arm64-iPhoneOS/build/lib/libuv.a \
    $BUILD_DIR/x86_64-iPhoneSimulator/build/lib/libuv.a \
    -output $LIB_DIR/libuv-ios.a

  die $? "not ok - unable to combine build artifacts"
  echo "ok - created fat library"

  unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
    CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET

  cp $LIB_DIR/libuv-ios.a $ASSETS_DIR/lib/libuv-ios.a
  die $? "not ok - could not copy fat library"
  echo "ok - copied fat library"
fi

_compile_libuv

cp $STAGING_DIR/build/lib/libuv.a $LIB_DIR
die $? "not ok - unable to build libuv"
echo "ok - built libuv for $platform ($target)"

mkdir -p  $ASSETS_DIR/uv/{src/unix,include}
cp -fr $BUILD_DIR/input/src/*.{c,h} $ASSETS_DIR/uv/src
cp -fr $BUILD_DIR/input/src/unix/*.{c,h} $ASSETS_DIR/uv/src/unix
cp -r $BUILD_DIR/input/include/* $ASSETS_DIR/uv/include
cp -r $BUILD_DIR/input/include/* $ASSETS_DIR/include
die $? "not ok - could not copy headers"
echo "ok - copied headers"
cd $WORK_DIR

_build
_install
