#!/usr/bin/env bash

VERSION_SSC=$(ssc -v)

VERSION_TXT=$(cat VERSION.txt)
VERSION_GIT=$(git rev-parse --short HEAD)
VERSION_EXPECTED="$VERSION_TXT ($VERSION_GIT)"

if [ "$VERSION_SSC" = "$VERSION_EXPECTED" ]; then
  echo "Version check has passed";
  exit 0;
else
  echo "Version check has failed";
  echo "Expected: $VERSION_EXPECTED";
  echo "Got: $VERSION_SSC";
  exit 1;
fi
