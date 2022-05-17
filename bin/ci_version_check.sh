#!/usr/bin/env bash

VERSION_OP=$(op -v)
VERSION_TXT=$(cat VERSION.txt)
VERSION_GIT=$(git rev-parse --short HEAD)

if [ "$VERSION_OP" = "$VERSION_TXT ($VERSION_GIT)" ]; then
  echo "Version check has passed";
  exit 0;
else
  echo "Version check has failed";
  echo "Expected: $VERSION_TXT ($VERSION_GIT)";
  echo "Got: $VERSION_OP";
  exit 1;
fi
