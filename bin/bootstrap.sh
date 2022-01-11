#!/usr/bin/env bash
set -e;

function _build {
  echo '• Building opkit'
  g++ src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
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
  sudo rm -rf /usr/local/lib/opkit

  echo '• Copying sources to /usr/local/lib/opkit/src'
  sudo mkdir -p /usr/local/lib/opkit
  sudo cp -r `pwd`/src/ /usr/local/lib/opkit

  if [ -d `pwd`/lib ]; then
    echo '• Copying libraries to /usr/local/lib/opkit/lib'
    sudo mkdir -p /usr/local/lib/opkit/lib
    sudo cp -r `pwd`/lib/ /usr/local/lib/opkit/lib
  fi

  echo '• Moving binary to /usr/local/bin'
  sudo mv `pwd`/bin/cli /usr/local/bin/opkit

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
  git clone --depth=1 git@github.com:operatortc/opkit.git $TMPD > /dev/null 2>&1

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
