#!/usr/bin/env bash

function dump_runtime_error() {
  # logcat blocks, output, switch to bg, wait a bit, quit
  $adb logcat | grep "E AndroidRuntime" & pid=$?
  sleep 1
  kill $?
  exit 1
}

id="co.socketsupply.socket.tests"

## Start application
[[ -z "$adb" ]] && adb="$(which adb 2>/dev/null)"
if [[ ! -f "$adb" ]]; then
  echo "adb not in path or ANDROID_HOME not set."
  exit 1
fi

"$adb" logcat -c
"$adb" shell am start -n "$id/.MainActivity" || exit $?


echo "polling for '$id' PID in adb"
# Additional timeout check to catch situation where app crashes before $pid is set
count=0
timeout=5
[[ -n "$CI" ]] && timeout=30
echo "App start timeout: $timeout"
while [ -z "$pid" ] && (( count < timeout )); do
  echo -n "$count..."
  pid="$($adb shell ps | grep "$id" | awk '{print $2}' 2>/dev/null)"
  ## Probe for application process ID
  sleep 1
  (( count++ ))
done

if (( count >= timeout )); then
  echo "timeout exceeded, assuming app crashed."
  dump_runtime_error
fi

echo "# got process pid: $pid"

## Process logs from 'adb logcat'
while read -r line; do

  if echo "$line" | grep "E AndroidRuntime: FATAL EXCEPTION: main" | grep "$pid"; then
    dump_runtime_error
  fi

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
