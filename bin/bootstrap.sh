#!/usr/bin/env bash
set -e;

PREFIX=${PREFIX:-$HOME}
PLATFORMPATH=""
ASSETS_DIR=""
SDKVERSION=""
LIPO=""
OLD_DIR=`pwd`
LIB_DIR=`pwd`/lib
BUILD_DIR=`pwd`/build

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
  fi
fi

if ! which autoconf >/dev/null 2>&1; then
  echo "• Error: Missing build tools, try 'brew install automake'"
  exit 1
fi

if ! which sudo > /dev/null 2>&1; then
  sudo () {
    $@
    return $?
  }
fi

function _build {
  echo "• Compiling sources"

  "$CXX" src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a \
    -DVERSION_HASH=`git rev-parse --short HEAD` \
    -DVERSION=`cat VERSION.txt` \

  if [ ! $? = 0 ]; then
    echo "• Unable to build. See trouble shooting guide in the README.md file"
    exit 1
  fi
  echo "• Built the ssc binary"
}

function _prepare {
  if [ ! -z "$LOCALAPPDATA" ]; then
    ASSETS_DIR="$LOCALAPPDATA/Programs/socketsupply"
  else
    ASSETS_DIR="$PREFIX/.config/socket-sdk"
  fi

  echo "• Preparing directories"
  rm -rf $LIB_DIR/uv
  rm -rf "$ASSETS_DIR"
  mkdir -p $ASSETS_DIR/{lib,src,include}

  # Shallow clone the main branch of libuv
  if [ ! -d "$BUILD_DIR" ]; then
  	git clone --depth=1 https://github.com/libuv/libuv.git build > /dev/null 2>&1;

    if [ ! $? = 0 ]; then
      echo "• Unable to clone. See trouble shooting guide in the README.md file"
      exit 1
    fi
  fi
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

  echo "• Finished. Type 'ssc -h' for help"
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
   echo "• Found SDK $sdk"
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

function _make_libuv {
  quiet make clean;
  quiet make -j 10;
  quiet make install;
}

function _compile_libuv {
  if [ -z "$1" ]; then
    quiet ./configure --prefix="$BUILD_DIR/output/desktop"
    _make_libuv
    return
  fi

  echo "• Building for $1 $2 (Running in background)"
  quiet sh autogen.sh

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

  quiet ./configure --prefix="$BUILD_DIR/output/$target" --host=$hosttarget-apple-darwin;

  if [ ! $? = 0 ]; then
    echo "• Failed to configure, see trouble shooting guide."
    exit 1
  fi

  _make_libuv
}

#
# Main
#
_prepare

cd $BUILD_DIR

if [ "$1" == "ios" ]; then
  if ! xcode-select -p >/dev/null 2>&1; then
    echo "Error: Xcode needs to be installed from the Mac App Store."
    exit 1
  fi

  SDKMINVERSION="8.0"
  export IPHONEOS_DEPLOYMENT_TARGET="8.0"

  LIPO=$(xcrun -sdk iphoneos -find lipo)
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

  _setSDKVersion iPhoneOS

  {
    _compile_libuv arm64 iPhoneOS &
    _compile_libuv i386 iPhoneSimulator &
    _compile_libuv x86_64 iPhoneSimulator &
  } | cat

  quiet $LIPO -create \
    $BUILD_DIR/output/arm64/lib/libuv.a \
    $BUILD_DIR/output/x86_64/lib/libuv.a \
    $BUILD_DIR/output/i386/lib/libuv.a \
    -output libuv.a

  if [ ! $? = 0 ]; then
    echo "• Unable to combine build artifacts, see trouble shooting guide."
    exit 1
  fi
else
  _compile_libuv
fi

# Copy the build into the project
quiet mkdir -p $LIB_DIR/uv/include;
cp libuv.a $LIB_DIR
cp -r $BUILD_DIR/include $ASSETS_DIR

cd $OLD_DIR

unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
  CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET

_build
_install
