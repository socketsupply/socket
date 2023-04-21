#!/usr/bin/env bash

function dump_runtime_error() {
  local logcat_pid
  # logcat blocks, output, switch to bg, wait a bit, quit
  $adb logcat | grep "E AndroidRuntime" & logcat_pid=$?
  sleep 3
  kill $logcat_pid
  exit 1
}

function dump_pid() {
  local app_pid="$1"
  local exit_code="$2"
  local logcat_pid

  [[ -z "$exit_code" ]] && exit_code=1
  # logcat blocks, output, switch to bg, wait a bit, quit
  $adb logcat --pid="$app_pid" & logcat_pid=$?
  sleep 1
  kill $logcat_pid
  exit $exit_code
}

function exit_on_adb_crash_signal() {
  while read -r line; do
    echo "'--------- beginning of crash' occured in adb output, exiting."
    dump_runtime_error
  done < <($adb logcat | grep -- "--------- beginning of crash")
}

declare poll_adb_watchdog_file
function watchdog_file_set() {
  poll_adb_watchdog_file="$(mktemp)"
}

function watchdog_file_clear() {
  exit_code=$1
  if [[ "$exit_code" != "0" ]]; then
    echo "$exit_code" > "$poll_adb_watchdog_file"
  else
    rm -r "$poll_adb_watchdog_file"
  fi
}

function watchdog_file_exists() {  
  if [ -f "$poll_adb_watchdog_file" ]; then
    data="$(cat "$poll_adb_watchdog_file")"
    if [ -z "$data" ]; then
      echo "0"
      return
    fi
  fi

  echo "1"
}

id="co.socketsupply.socket.tests"

## Start application
[[ -z "$adb" ]] && adb="$(which adb 2>/dev/null)"
if [[ ! -f "$adb" ]]; then
  echo "adb not in path or ANDROID_HOME not set."
  exit 1
fi

"$adb" logcat -c
exit_on_adb_crash_signal & adb_crash_pid=$!
"$adb" shell am start -n "$id/.MainActivity" || exit $?


echo "polling for '$id' PID in adb"
# Additional timeout check to catch situation where app crashes before $pid is set
count=0
count_output=""
timeout=5
[[ -n "$CI" ]] && timeout=30
echo "App start timeout: $timeout"
echo ""
while [ -z "$pid" ] && (( count < timeout )); do
  count_output="$count_output$count..." # Ouput doesn't flush in CI with echo -n, rebuild line
  
  [[ -n "$CI" ]] && echo "$count..."
  [[ -z "$CI" ]] && echo -e "\e[1A\e[K$count_output" # Replace previous line, doesn't work in CI
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
watchdog_file_set
# kill $adb_crash_pid
while read -r line; do

  if echo "$line" | grep "E AndroidRuntime: FATAL EXCEPTION: main" | grep "$pid"; then
    # dump_runtime_error terminates
    echo "dump_runtime_error"
    dump_runtime_error
  fi

  if echo "$line" | grep 'Fatal signal'; then
    # dump_pid terminates
    echo "dump_pid"
    watchdog_file_clear 1
    dump_pid "$pid"
  fi

  if grep 'Console :' < <(echo "$line") >/dev/null; then
    line="$(echo "$line" | sed 's/.*[D|E|I|W] Console :\s*//g')"

    if [[ "$line" =~ __EXIT_SIGNAL__ ]]; then
      exit_signal="${line/__EXIT_SIGNAL__=/}"
      echo "Received exit signal line: $line, signal: $exit_signal"
      watchdog_file_clear "$exit_signal"
      exit "$exit_signal"
    fi

    echo "$line"

    if [[ "$line" == "# ok" ]]; then
      watchdog_file_clear 0
      exit
    fi

    if [[ "$line" == "# fail" ]]; then
      watchdog_file_clear 1
      exit 1
    fi
  fi
done < <($adb logcat --pid="$pid") & logcat_pid=$!

# Handle situation where logcat loop doesn't catch exit, because eg an unexpected error condition occured that isn't handled by the text processing above
# This section dumps the entire process log after it has gone away, note that we don't want the entire log on failed tests that were successfully reported
count=0
timeout=300
echo "Waiting 5m before aborting tests..."

# while [[ "$(watchdog_file_exists)" == "0" ]]; do
while (( count < timeout )) ; do
  if [[ "$(watchdog_file_exists)" != "0" ]]; then
    break
  fi
  
  (( count > 0 )) && (( count % 30 == 0 )) && echo "Timeout count: $count/$timeout"
  sleep 1
  (( count++ ))
done

if [[ "$(watchdog_file_exists)" == "0" ]]; then
  echo "Timeout exceeded."
else
  exit_code="$(cat "$poll_adb_watchdog_file" 2>/dev/null)"
  if [[ -z "$exit_code" ]]; then
    date
    dump_pid "$logcat_pid" 25
  else
    exit $exit_code
  fi
fi

echo "poll-adb-logcat.sh exiting without signal from adb loop"
wait $logcat_pid
exit 254