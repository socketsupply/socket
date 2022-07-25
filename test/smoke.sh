#!/bin/bash
PLATFORM=`uname -s`

function quiet () {
  "$@" > /dev/null 2>&1
}

function die {
  if [ ! $1 = 0 ]; then
    echo "not ok - $2" && exit 1
  fi
  echo "ok - $2"
}

echo "TAP version 13"
echo "1..N"

TEST=true ./bin/install.sh ios
die $? "the cli tool was built"

mkdir -p test/tmp
cd test/tmp

echo # the init command creates dirs and ouputs files

../../bin/cli init

quiet stat src/index.html
die $? "the index.html file exists"

quiet stat ssc.config
die $? "the config file exists"

quiet ../../bin/cli compile .
die $? "the compile command executed"

#
# MacOS
#
if [ "$PLATFORM" == "Darwin" ]; then
  quiet stat build/beepboop-dev.app/Contents/MacOS/boop-dev
  die $? "the compile command created a binary in the correct location"

  osascript <<EOD
    set pathToApp to "build/beepboop-dev.app"
    tell application pathToApp to activate
    delay 1
    tell application pathToApp to quit
EOD
fi

#
# Linux
#
if [ "$PLATFORM" == "linux" ]; then
  echo "TODO"
  exit 1
fi

#
# Windows (via WSL)
#
if [ "`uname -a`" == *"indows"* ]; then
  echo "TODO"
  exit 1
fi

cd ../..
rm -rf test/tmp
