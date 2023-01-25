#!/usr/bin/env bash

declare id=""
declare root=""

id="co.socketsupply.socket.tests"
root="$(dirname "$(dirname "${BASH_SOURCE[0]}")")"

adb uninstall "$id"

ssc build --headless --platform=android -r -o .

adb shell rm -rf "/data/local/tmp/ssc-socket-test-fixtures"
adb push "$root/fixtures/" "/data/local/tmp/ssc-socket-test-fixtures"

"$root/scripts/poll-adb-logcat.sh"
