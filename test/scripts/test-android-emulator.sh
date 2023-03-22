#!/usr/bin/env bash

id="co.socketsupply.socket.tests"
adb="$ANDROID_HOME/platform-tools/adb"
root="$(CDPATH='' cd -- "$(dirname "$(dirname -- "$0")")" && pwd)"

${SHELL:-sh} "$root/scripts/bootstrap-android-emulator.sh" &

echo "info: Waiting for Android Emulator to boot"
while ! "$adb" shell getprop sys.boot_completed >/dev/null 2>&1 ; do
  sleep 0.5
done
echo "info: Android Emulator booted"

"$adb" uninstall "$id"
ssc build -r -o --prod --headless --platform=android-emulator || {
  rc=$?
  echo "info: Shutting Android Emulator"
  "$adb" devices | grep emulator | cut -f1 | while read -r line; do
    "$adb" -s "$line" emu kill
  done
  exit "$rc"
}

"$adb" shell rm -rf "/data/local/tmp/ssc-socket-test-fixtures"
"$adb" push "$root/fixtures/" "/data/local/tmp/ssc-socket-test-fixtures"

${SHELL:-sh} "$root/scripts/poll-adb-logcat.sh"

echo "info: Shutting Android Emulator"
"$adb" devices | grep emulator | cut -f1 | while read -r line; do
  "$adb" -s "$line" emu kill
done

"$adb" kill-server
