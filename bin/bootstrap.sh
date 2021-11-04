#!/usr/bin/env bash
set -e;

if [ ! "$CXX" ]; then
  echo '• Warning: $CXX environment variable not set, assuming "/usr/bin/g++"'
  CXX=/usr/bin/g++
fi

function build {
  `echo $CXX` src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a\
    -DVERSION=`git rev-parse --short HEAD`

  if [ ! $? = 0 ]; then
    echo '• Unable to build. See trouble shooting guide in the README.md file'
    exit 1
  fi
  echo '• Success'
}

#
# Install - when this script is called with a parameter
#
if [ "$1" ]; then
  if [ -z "$2" ]; then
    TMPD=$(mktemp -d)

    echo '• Cloning from Github'
    git clone --depth=1 git@github.com:operatortc/opkit.git $TMPD > /dev/null 2>&1

    if [ ! $? = 0 ]; then
      echo "• Unable to clone"
      exit 1
    fi

    cd $TMPD
  fi

  echo '• Building'
  build

  sudo rm -rf /usr/local/lib/opkit

  echo '• Copying sources to /usr/local/lib/opkit'
  sudo mkdir -p /usr/local/lib/opkit
  sudo cp -r `pwd`/src/ /usr/local/lib/opkit/src

  echo '• Moving binary to /usr/local/bin'
  sudo mv `pwd`/bin/cli /usr/local/bin/opkit

  if [ ! $? = 0 ]; then
    echo '• Unable to move file into place'
    exit 1
  fi

  echo -e '• Finished. Type "opkit -h" for help'
  exit 0
fi

#
# Build - when being run from the source tree (developer mode)
#
echo '• Compiling build program'
build
