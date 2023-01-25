#!/usr/bin/env bash

declare id=""
declare pid=""

id="co.socketsupply.socket.tests"

## Start application
adb shell am start -n "$id/.MainActivity" || exit $?

echo "polling for '$id' PID in adb"
while [ -z "$pid" ]; do
  ## Probe for application process ID
  pid="$(adb shell ps | grep "$id" | awk '{print $2}' 2>/dev/null)"
  sleep 1s
done

## Process logs from 'adb logcat'
while read -r line; do
  if grep 'Console' < <(echo "$line") >/dev/null; then
    line="$(echo "$line" | sed 's/.*Console:\s*//g')"

    if [[ "$line" =~ __EXIT_SIGNAL__ ]]; then
      status="${line//__EXIT_SIGNAL__=/}"
      exit "$status"
    fi

    echo "$line"

    if [ "$line" == "# ok" ]; then
      exit
    fi

    if [ "$line" == "# fail" ]; then
      exit 1
    fi
  fi
done < <(adb logcat --pid="$pid")
