#!/bin/bash
PLATFORM=`uname -s`

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
source "$root/bin/functions.sh"

echo "TAP version 13"
echo "1..N"

TEST=true ./bin/install.sh ios
die $? "the cli tool was built"

mkdir -p test/tmp
cd test/tmp

echo # the init command creates dirs and ouputs files

quiet ../../bin/cli init

quiet stat src/index.html
die $? "the index.html file exists"

quiet stat socket.ini
die $? "the config file exists"

quiet ../../bin/cli compile .
die $? "the compile command executed"

#
# MacOS
#
if [ "$PLATFORM" == "Darwin" ]; then
  quiet stat build/beepboop-dev.app/Contents/MacOS/boop-dev
  die $? "the compile command created a binary in the correct location"

  pathToApp=$PWD/build/beepboop-dev.app

  osascript <<EOD
    set app to POSIX file "$pathToApp" as alias
    tell application app to activate
    delay 1
    tell application app to quit
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
