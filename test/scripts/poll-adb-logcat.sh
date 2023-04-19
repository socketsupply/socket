#!/usr/bin/env bash

id="co.socketsupply.socket.tests"

## Start application
[[ -z "$adb" ]] && adb="$(which adb 2>/dev/null)"
if [[ ! -f "$adb" ]]; then
  echo "adb not in path or ANDROID_HOME not set."
  exit 1
fi

"$adb" shell am start -n "$id/.MainActivity" || exit $?

echo "polling for '$id' PID in adb"
while [ -z "$pid" ]; do
  ## Probe for application process ID
  pid="$($adb shell ps | grep "$id" | awk '{print $2}' 2>/dev/null)"
  sleep 1
done

## Process logs from 'adb logcat'
while read -r line; do
  if grep 'Console :' < <(echo "$line") >/dev/null; then
    line="$(echo "$line" | sed 's/.*[D|E|I|W] Console :\s*//g')"

    if [[ "$line" =~ __EXIT_SIGNAL__ ]]; then
      exit_signal="${line/__EXIT_SIGNAL__=/}"
      exit "$exit_signal"
    fi

    echo "$line"

    if [[ "$line" == "# ok" ]]; then
      exit
    fi

    if [[ "$line" == "# fail" ]]; then
      exit 1
    fi
  fi
done < <($adb logcat --pid="$pid")
