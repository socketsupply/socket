#!/usr/bin/env bash
set -e;

if [ ! $CXX ]; then
  echo '• Warning: $CXX environment variable not set, assuming "/usr/bin/g++"'
  CXX=/usr/bin/g++
fi

function build {
  `echo $CXX` src/build.cc -o bin/build -std=c++2a -stdlib=libc++
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
    git clone --depth=1 git@github.com:optoolco/opkit.git $TMPD > /dev/null 2>&1

    if [ ! $? = 0 ]; then
      echo "• Unable to clone"
      exit 1
    fi

    # enter the temp dir and run the build of the build tool
    cd $TMPD
  fi

  echo '• Building'
  build

  # create a symlink to the binary so it can be run anywhere
  echo '• Moving binary to /usr/local/bin'
  mv `pwd`/bin/build /usr/local/bin/opkit

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
