#!/usr/bin/env bash

set -e;
DEST=/usr/local/lib/opkit

echo '• Cloning from Github'
git clone https://github.com/optoolco/opkit $DEST > /dev/null 2>&1

if [ ! $? = 0 ]; then
  echo "• Unable to clone"
  exit 1
fi

cd $DEST

echo '• Building'
g++ src/build.cc -o bin/opkit -std=c++2a

if [ ! $? = 0 ]; then
  echo "• Unable to build"
  exit 1
fi

echo '• Attempting to create a symlink in /usr/loca/bin'
ln -s bin/opkit /usr/local/bin/opkit

if [ ! $? = 0 ]; then
  echo "• Unable to create symlink"
  exit 1
fi

echo '• Finished. Type "opkit" for help.'
