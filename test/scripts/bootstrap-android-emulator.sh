#!/usr/bin/env bash

# enable async operation by writing exit code to a file for callee to test
# call this to identify that emulator is about to be called, but don't exit script
function write_code() {
  local exit_code="$1"
  [[ -n "$error_file" ]] && echo "$exit_code" > "$error_file"
}

# Call this on error, script will terminate
error_file=$1
function exit_and_write_code() {
  local exit_code="$1"
  write_code "$exit_code"
  exit "$exit_code"
}

declare rc

ssc_env=""
if [ -f ".ssc.env" ]; then
  ssc_env=".ssc.env"
elif [ -f "../.ssc.env" ]; then
  ssc_env="../.ssc.env"
fi

if [ -n "$ssc_env" ]; then
  echo "# Sourcing $ssc_env"
  source "$ssc_env"
  sdkmanager="$ANDROID_HOME/$ANDROID_SDK_MANAGER"
  avdmanager="$(dirname $sdkmanager)/avdmanager"
  export ANDROID_HOME
  export JAVA_HOME
fi

if [ -z "$ANDROID_HOME" ]; then
  if test -d "$HOME/android"; then
    ANDROID_HOME="$HOME/android"
  elif test -d "$HOME/Android"; then
    ANDROID_HOME="$HOME/Android"
  elif test -d "$HOME/Library/Android/sdk"; then
    ANDROID_HOME="$HOME/Library/Android/sdk"
  elif test -d "$HOME/Library/Android"; then
    ANDROID_HOME="$HOME/Library/Android"
  elif test -d "$HOME/.android/sdk"; then
    ANDROID_HOME="$HOME/.android/sdk"
  elif test -d "$HOME/.android"; then
    ANDROID_HOME="$HOME/.android"
  fi
fi

emulator="$(which emulator 2>/dev/null)"
[[ -z "$avdmanager" ]] && avdmanager="$(which avdmanager 2>/dev/null)"
[[ -z "$sdkmanager" ]] && sdkmanager="$(which sdkmanager 2>/dev/null)"

if [ -z "$emulator" ]; then
  emulator="$ANDROID_HOME/emulator/emulator"
fi

if [ -z "$avdmanager" ]; then
  avdmanager="$ANDROID_HOME/cmdline-tools/tools/bin/avdmanager"
fi

if [ -z "$sdkmanager" ]; then
  sdkmanager="$ANDROID_HOME/cmdline-tools/tools/bin/sdkmanager"
fi

if [ ! -f "$avdmanager" ]; then
  echo "not ok - Unable to locate avdmanager: $avdmanager"
  exit_and_write_code 1
fi

if [ ! -f "$sdkmanager" ]; then
  echo "not ok - Unable to locate sdkmanager."
  exit_and_write_code 1
fi

if [ ! -f "$emulator" ]; then
  echo "not ok - Unable to locate emulator."
  exit_and_write_code 1
fi

write_code 0

if ! "$avdmanager" list avd | grep 'Name: SSCAVD$'; then
  pkg="system-images;android-33;google_apis;$(uname -m | sed -E 's/(arm64|aarch64)/arm64-v8a/g')"
  "$sdkmanager" "$pkg"
  rc=$?
  (( !rc )) && exit_and_write_code $rc
  echo yes | "$avdmanager" --clear-cache create avd -n SSCAVD -k "$pkg" -d 1 --force
  rc=$?
  (( !rc )) && exit_and_write_code $rc
fi

"$emulator" @SSCAVD         \
  $EMULATOR_FLAGS           \
  -gpu swiftshader_indirect \
  -camera-back none         \
  -no-boot-anim             \
  -no-window                \
  -noaudio                  \
  >/dev/null

exit_and_write_code $?
