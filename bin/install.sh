#!/usr/bin/env bash
set -e;

PREFIX=${PREFIX:-$HOME}
PLATFORMPATH=""
ASSETS_DIR=""
SDKVERSION=""
LIPO=""
WORK_DIR=`pwd`
LIB_DIR=$WORK_DIR/lib
BUILD_DIR=$WORK_DIR/build

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
    echo "not ok - could not determine \$CXX environment variable"
    exit 1
  fi
fi

function quiet () {
  "$@" > /dev/null 2>&1
}

if ! quiet which sudo; then
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

quiet which make
die $? "not ok - missing build tools, try 'brew install automake'"

quiet which autoconf
die $? "not ok - missing build tools, try 'brew install automake'"

function _build {
  echo "# building cli for desktop (`uname -m`)..."

  "$CXX" src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a \
    -DVERSION_HASH=`git rev-parse --short HEAD` \
    -DVERSION=`cat VERSION.txt` \

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

  if [ ! -d "$BUILD_DIR" ]; then
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

  echo "# moving binary to $PREFIX/bin (prompting to copy file into directory)"
  sudo mv `pwd`/bin/cli "/usr/local/bin/ssc"

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

    cp $STAGING_DIR/build/lib/libuv.a $LIB_DIR
    die $? "not ok - unable to build libuv"
    echo "ok - built libuv for $platform ($target)"

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

  die $? "not ok - failed to configure"

  quiet make -j8
  quiet make install

  cd $BUILD_DIR
  echo "ok - built for $target"
}

_prepare
cd $BUILD_DIR

if [ "$1" == "ios" ]; then
  quiet xcode-select -p
  die $? "not ok - xcode needs to be installed from the mac app store"

  SDKMINVERSION="8.0"
  export IPHONEOS_DEPLOYMENT_TARGET="8.0"

  LIPO=$(xcrun -sdk iphoneos -find lipo)
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

  _setSDKVersion iPhoneOS

  pids=""
  _compile_libuv arm64 iPhoneOS & pids="$pids $!"
  _compile_libuv i386 iPhoneSimulator & pids="$pids $!"
  _compile_libuv x86_64 iPhoneSimulator & pids="$pids $!"

  for pid in $pids; do wait $pid; done

  quiet $LIPO -create \
    $BUILD_DIR/arm64-iPhoneOS/build/lib/libuv.a \
    $BUILD_DIR/x86_64-iPhoneSimulator/build/lib/libuv.a \
    $BUILD_DIR/i386-iPhoneSimulator/build/lib/libuv.a \
    -output $LIB_DIR/libuv.a

  die $? "not ok - unable to combine build artifacts"
  echo "ok - created fat library"

  unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
    CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET

  cp $LIB_DIR/libuv.a $ASSETS_DIR/lib/libuv.a
  die $? "not ok - could not copy fat library"
  echo "ok - copied fat library"
else
  _compile_libuv
fi

cp -r $BUILD_DIR/input/include/ $ASSETS_DIR/include
die $? "not ok - could not copy headers"
echo "ok - copied headers"
cd $WORK_DIR

_build
_install
