#
# TODO convert this to a brew package
#
#!/usr/bin/env bash
set -e;

PREFIX=${PREFIX:-"/usr/local"}

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
    echo "• error: Could not determine \$CXX environment variable"
    exit 1
  else
    echo "• Warning: \$CXX environment variable not set, assuming '$CXX'"
fi
  fi

if ! which sudo > /dev/null 2>&1; then
  function sudo {
    $@
    return $?
  }
fi

function _build {
  echo '• Building op'
  "$CXX" src/cli.cc ${CXX_FLAGS} ${CXXFLAGS} \
    -o bin/cli \
    -std=c++2a \
    -DVERSION_HASH=`git rev-parse --short HEAD` \
    -DVERSION=`cat VERSION.txt` \

  if [ ! $? = 0 ]; then
    echo '• Unable to build. See trouble shooting guide in the README.md file'
    exit 1
  fi
  echo '• Success'
}

function _install {
  local libdir=""

  ## must be a windows environment
  if [ ! -z "$LOCALAPPDATA" ]; then
    libdir="$LOCALAPPDATA/Programs/socketsupply"
  else
    libdir="$PREFIX/lib/op"
  fi

  echo "• Installing op"
  sudo rm -rf "$libdir"

  sudo mkdir -p "$libdir"
  sudo cp -r `pwd`/src "$libdir"

  echo "• Copying sources to $libdir/src"
  if [ -d `pwd`/lib ]; then
    echo "• Copying libraries to $libdir/lib"
    sudo mkdir -p "$libdir/lib"
    sudo cp -r `pwd`/lib/ "$libdir/lib"
  fi

  echo "• Moving binary to $PREFIX/bin"
  sudo mv `pwd`/bin/cli "$PREFIX/bin/op"

  if [ ! $? = 0 ]; then
    echo "• Unable to move binary into place"
    exit 1
  fi

  echo -e '• Finished. Type "op -h" for help'
  exit 0
}

#
# Clone
#
if [ -z "$1" ]; then
  TMPD=$(mktemp -d)

  echo '• Cloning from Github'
  git clone --depth=1 git@github.com:socketsupply/operatorframework.git $TMPD > /dev/null 2>&1

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
