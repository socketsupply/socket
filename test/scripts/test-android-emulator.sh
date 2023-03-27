#!/usr/bin/env bash


root="$(CDPATH='' cd -- "$(dirname "$(dirname -- "$0")")" && pwd)"

if [[ -n $1 ]]; then
  host=$1
else
  host="$(uname -s)"
fi

if [[ "$host" = "Linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    echo >&2 "error: WSL is not supported."
    exit 1
  fi
elif [[ "$host" == *"MINGW64_NT"* ]]; then
  host="Win32"
elif [[ "$host" == *"MSYS_NT"* ]]; then
  host="Win32"
fi

ssc_env=""
if [ -f ".ssc.env" ]; then
  ssc_env=".ssc.env"
elif [ -f "../.ssc.env" ]; then
  ssc_env="../.ssc.env"
fi

if [ -n "$ssc_env" ]; then
  echo "# Sourcing $ssc_env"
  source "$ssc_env"
  export ANDROID_HOME
  export ANDROID_SDK_MANAGER
  export JAVA_HOME
fi

id="co.socketsupply.socket.tests"
adb="$(which adb 2>/dev/null)"
[[ -z "$adb" ]] && adb="$ANDROID_HOME/platform-tools/adb"
[[ ! -f "$adb" ]] && (echo "adb not in path or ANDROID_HOME not set."; exit 1)
export adb

temp="$(mktemp)"
${SHELL:-sh} -c "$root/scripts/bootstrap-android-emulator.sh $temp" & bootstrap_pid=$!

bootstrap_exit_code=""
while [ -z "$bootstrap_exit_code" ]; do
  # Wait for exit code
  bootstrap_exit_code="$(cat "$temp")"
  sleep 0.5
done


[ "$bootstrap_exit_code" != "0" ] && (rm "$temp"; exit "$bootstrap_exit_code")

# reset bootstrap exit code, we will use it again
bootstrap_exit_code=""
echo > "$temp"

# Start building, OK to fail on install.
# ssc build --headless --platform=android -r -o & build_pid=$?

echo "info: Waiting for Android Emulator to boot"

boot_completed=""
while [[ "$boot_completed" != "1" ]] ; do
  boot_completed="$($adb shell getprop sys.boot_completed 2>/dev/null)"
  # reliable cross platform method of waiting for background process asynchronously
  bootstrap_exit_code="$(cat "$temp")"
  if [[ -n "$bootstrap_exit_code" ]] && [[ "$bootstrap_exit_code" != "0" ]]; then
    # emuator already exited
    echo "Android Emulator failed to boot."
    wait $bootstrap_pid
    rm "$temp"
    exit $?
  fi
  sleep 0.5
done
echo "info: Android Emulator booted"

rm "$temp"

"$adb" uninstall "$id"

echo "Removing old fixtures..."
"$adb" shell rm -rf "/data/local/tmp/ssc-socket-test-fixtures"
echo "Pushing fixtures..."

fixtures_path="$root/fixtures"
# adb on windows doesn't handle incorrect slashes (hangs)
if [[ "$host" != "Win32" ]]; then
  "$adb" push "$fixtures_path" "/data/local/tmp/ssc-socket-test-fixtures"
else
  # adb push just doesn't work under mingw (It hangs, attempts to prefix DEST path with c:\Program Files\Git...)
  fixtures_path="${fixtures_path/"$(pwd)/"/}"
  # change to a relative native path (So we don't have to fix drive letter)
  fixtures_path="${fixtures_path//\//\\}"
  "$COMSPEC" "/k $adb push $fixtures_path /data/local/tmp/ssc-socket-test-fixtures && exit "
fi

ssc build --headless --platform=android -r -o || {
  rc=$?
  echo "info: Shutting Android Emulator"
  "$adb" devices | grep emulator | cut -f1 | while read -r line; do
    "$adb" -s "$line" emu kill
  done
  exit "$rc"
}

${SHELL:-sh} "$root/scripts/poll-adb-logcat.sh"

echo "info: Shutting Android Emulator"
"$adb" devices | grep emulator | cut -f1 | while read -r line; do
  "$adb" -s "$line" emu kill
done

"$adb" kill-server
