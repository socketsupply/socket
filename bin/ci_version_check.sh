#!/usr/bin/env bash

VERSION_SSC=$(ssc -v | head -n 1)

VERSION_TXT=$(cat VERSION.txt)
VERSION_GIT=$(git rev-parse --short=8 HEAD)
VERSION_EXPECTED="$VERSION_TXT ($VERSION_GIT)"

if [ "$VERSION_SSC" = "$VERSION_EXPECTED" ]; then
  echo "Version check has passed";
else
  echo "Version check has failed";
  echo "Expected: $VERSION_EXPECTED";
  echo "Got: $VERSION_SSC";
  exit 1;
fi

BASE_LIST=($(echo $VERSION_TXT | tr '.' ' '))

V_MAJOR=${BASE_LIST[0]}
V_MINOR=${BASE_LIST[1]}

VERSION_NODE=$(npm show ./npm/packages/@socketsupply/socket-node version)

BASE_LIST=($(echo $VERSION_NODE | tr '.' ' '))

V_MAJOR_NODE=${BASE_LIST[0]}
V_MINOR_NODE=${BASE_LIST[1]}

if [ "$V_MAJOR" -ne "$V_MAJOR_NODE" ] || [ "$V_MINOR" -ne "$V_MINOR_NODE" ]; then
    echo "Version of @socketsupply/socket-node is not in sync with @socketsupply/socket";
    echo "@socketsupply/socket version is $VERSION_TXT";
    echo "@socketsupply/socket-node version is $VERSION_NODE";
    exit 1;
fi
