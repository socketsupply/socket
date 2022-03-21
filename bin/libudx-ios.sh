#!/usr/bin/env bash
#
# This will re-compile libudx for iOS (and the iOS simulator).
#

PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

export IPHONEOS_DEPLOYMENT_TARGET="8.0"
OLD_CWD=`pwd`
BUILD_DIR=`pwd`/lib/build
rm -rf $BUILD_DIR
rm -rf `pwd`/lib/udx

#
# Shallow clone the main branch of libudx.
#
git clone --depth=1 git@github.com:socketsupply/libudx.git lib/build
cd $BUILD_DIR

SetSDKVersion() {
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

Compile() {
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
  export CFLAGS="-fembed-bitcode -fPIC -fno-rtti -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -miphoneos-version-min=$SDKMINVERSION"
  export AR=$(xcrun -sdk iphoneos -find ar)
  export RANLIB=$(xcrun -sdk iphoneos -find ranlib)
  export CPPFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -miphoneos-version-min=$SDKMINVERSION"
  export LDFLAGS="-Wc,-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk"
  export OPTS="-I$PWD/../uv/include"

  $CC $CFLAGS $CPPFLAGS -o cirbuf.o -c ./src/cirbuf.c $OPTS
  $CC $CFLAGS $CPPFLAGS -o fifo.o -c ./src/fifo.c $OPTS
  $CC $CFLAGS $CPPFLAGS -o io.o -c ./src/cirbuf.c $OPTS
  $CC $CFLAGS $CPPFLAGS -o io_posix.o -c ./src/fifo.c $OPTS
  $CC $CFLAGS $CPPFLAGS -o udx.o -c ./src/udx.c $OPTS
  $AR rvs libudx.a utils.o cirbuf.o fifo.o udx.o

  mkdir -p $BUILD_DIR/output/$target
  mv libudx.a $BUILD_DIR/output/$target/libudx.a
}

SetSDKVersion iPhoneOS
SDKMINVERSION="8.0"

#
# Build artifacts for all platforms
#
Compile armv7 iPhoneOS
Compile armv7s iPhoneOS
Compile arm64 iPhoneOS
Compile i386 iPhoneSimulator
Compile x86_64 iPhoneSimulator

#
# Combine the build artifacts
#
LIPO=$(xcrun -sdk iphoneos -find lipo)

$LIPO -create \
  $BUILD_DIR/output/armv7/libudx.a \
  $BUILD_DIR/output/armv7s/libudx.a \
  $BUILD_DIR/output/arm64/libudx.a \
  $BUILD_DIR/output/x86_64/libudx.a \
  $BUILD_DIR/output/i386/libudx.a \
  -output libudx.a

$LIPO -info libudx.a

#
# Copy the build into the project and delete leftover build artifacts.
#
DEST_DIR=$BUILD_DIR/../udx
mkdir $DEST_DIR

cp libudx.a $DEST_DIR
mkdir -p $DEST_DIR/include
cp -r $BUILD_DIR/src/*.h $DEST_DIR/include

rm -rf $BUILD_DIR
cd $OLD_CWD
