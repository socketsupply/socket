#!/usr/bin/env bash
set -e;

PREFIX=${PREFIX:-"/usr/local"}

if [ ! "$CXX" ]; then
  if which g++ >/dev/null 2>&1; then
    CXX="$(which g++)"
  elif which clang++ >/dev/null 2>&1; then
    CXX="$(which clang++)"
  fi

  if [ ! "$CXX" ]; then
    echo "• error: Could not determine \$CXX environment variable"
    exit 1
  else
    echo "• Warning: \$CXX environment variable not set, assuming '$CXX'"
fi
  fi

if ! which sudo > /dev/null 2>&1; then
  function sudo {
    eval "$@"
    return $?
  }
fi

function _build {
  echo '• Building opkit'
  "$CXX" src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a\
    -DVERSION=`git rev-parse --short HEAD`

  if [ ! $? = 0 ]; then
    echo '• Unable to build. See trouble shooting guide in the README.md file'
    exit 1
  fi
  echo '• Success'
}

function _install {
  echo '• Installing opkit'
  sudo rm -rf "$PREFIX/lib/opkit"

  echo '• Copying sources to $PREFIX/lib/opkit/src'
  sudo mkdir -p "$PREFIX/lib/opkit"
  sudo cp -r `pwd`/src "$PREFIX/lib/opkit"

  if [ -d `pwd`/lib ]; then
    echo '• Copying libraries to $PREFIX/lib/opkit/lib'
    sudo mkdir -p "$PREFIX/lib/opkit/lib"
    sudo cp -r `pwd`/lib/ "$PREFIX/lib/opkit/lib"
  fi

  echo '• Moving binary to $PREFIX/bin'
  sudo mv `pwd`/bin/cli "$PREFIX/bin/opkit"

  if [ ! $? = 0 ]; then
    echo '• Unable to move binary into place'
    exit 1
  fi

  echo -e '• Finished. Type "opkit -h" for help'
  exit 0
}

#
# Clone
#
if [ -z "$1" ]; then
  TMPD=$(mktemp -d)

  echo '• Cloning from Github'
  git clone --depth=1 git@github.com:socketsupply/opkit.git $TMPD > /dev/null 2>&1

  if [ ! $? = 0 ]; then
    echo "• Unable to clone"
    exit 1
  fi

  cd $TMPD
fi

#
# Build and Install
#
_build
_install
