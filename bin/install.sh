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

if ! which autoconf >/dev/null 2>&1; then
  echo "not ok - missing build tools, try 'brew install automake'"
  exit 1
fi

if ! which sudo > /dev/null 2>&1; then
  sudo () {
    $@
    return $?
  }
fi

function _build {
  echo "# building cli for `uname -m`..."

  "$CXX" src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a \
    -DVERSION_HASH=`git rev-parse --short HEAD` \
    -DVERSION=`cat VERSION.txt` \

  if [ ! $? = 0 ]; then
    echo "not ok - unable to build. See trouble shooting guide in the README.md file"
    exit 1
  fi
  echo "ok - built the cli"
}

function _prepare {
  if [ ! -z "$LOCALAPPDATA" ]; then
    ASSETS_DIR="$LOCALAPPDATA/Programs/socketsupply"
  else
    ASSETS_DIR="$PREFIX/.config/socket-sdk"
  fi

  echo "# preparing directories..."
  #rm -rf $LIB_DIR/uv
  rm -rf "$ASSETS_DIR"
  mkdir -p $ASSETS_DIR/{lib,src,include}

  # Shallow clone the main branch of libuv
  if [ ! -d "$BUILD_DIR" ]; then
  	git clone --depth=1 https://github.com/libuv/libuv.git $BUILD_DIR/input > /dev/null 2>&1
    rm -rf $BUILD_DIR/input/.git

    if [ ! $? = 0 ]; then
      echo "not ok - unable to clone. See trouble shooting guide in the README.md file"
      exit 1
    fi
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

  if [ ! $? = 0 ]; then
    echo "not ok - unable to move binary into place"
    exit 1
  fi

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

function quiet () {
  "$@" > /dev/null 2>&1
}

function _compile_libuv {
  if [ -z "$1" ]; then
    quiet ./configure --prefix="$BUILD_DIR/output/desktop"
    quiet make -j8
    return
  fi

  echo "# building for $1 ($2)..."
  local STAGING_DIR=$BUILD_DIR/$1-$2

  if [ ! -d "$STAGING_DIR" ]; then
    cp -r $BUILD_DIR/input/ $STAGING_DIR
    cd $STAGING_DIR
 	  quiet sh autogen.sh
  else
    cd $STAGING_DIR
  fi

  target=$1
  hosttarget=$1
  platform=$2

  if [[ $hosttarget == "arm64" ]]; then
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

  quiet ./configure --prefix="$BUILD_DIR/output/$target" --host=$hosttarget-apple-darwin

  if [ ! $? = 0 ]; then
    echo "not ok - failed to configure, see trouble shooting guide"
    exit 1
  fi

  quiet make -j8

  cd $BUILD_DIR
  echo "ok - built for $target"
}

_prepare
cd $BUILD_DIR

if [ "$1" == "ios" ]; then
  if ! xcode-select -p >/dev/null 2>&1; then
    echo "not ok - xcode needs to be installed from the mac app store"
    exit 1
  fi

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
    $BUILD_DIR/output/arm64/lib/libuv.a \
    $BUILD_DIR/output/x86_64/lib/libuv.a \
    $BUILD_DIR/output/i386/lib/libuv.a \
    -output libuv.a

  if [ ! $? = 0 ]; then
    echo "not ok - unable to combine build artifacts, see trouble shooting guide"
    exit 1
  fi

  unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
    CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET
else
  _compile_libuv
fi

cp libuv.a $LIB_DIR
cp -r input/include/ $ASSETS_DIR/include
cd $WORK_DIR

_build
_install
