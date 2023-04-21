#!/usr/bin/env bash

# 'namespaced' root
bae_root="$(CDPATH='' cd -- "$(dirname "$(dirname -- "$0")")" && pwd)"

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
if [ -f "$bae_root/.ssc.env" ]; then
  ssc_env="$bae_root/.ssc.env"
elif [ -f ".ssc.env" ]; then
  ssc_env=".ssc.env"
elif [ -f "../.ssc.env" ]; then
  ssc_env="../.ssc.env"
fi

if [ -n "$ssc_env" ]; then
  echo "# Sourcing $ssc_env"
  source "$ssc_env"
fi

if [ -n "$ANDROID_SDK_MANAGER" ]; then
  sdkmanager="$ANDROID_HOME/$ANDROID_SDK_MANAGER"
  bn="$(basename "$sdkmanager")"
  ext=""
  # Only add the result of extension filter if basename contains '.', otherwise it will contain 'sdkmanager'
  [[ "$bn" == *"."* ]] && [[ "$ext" != "$sdkmanager" ]] && ext=".${bn##*.}"
  avdmanager="$(dirname "$sdkmanager")/avdmanager$ext"
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
  echo "Downloading AVD image..."
  pkg="system-images;android-33;google_apis;$(uname -m | sed -E 's/(arm64|aarch64)/arm64-v8a/g')"
  yes | "$sdkmanager" "$pkg"
  rc=$?
  (( rc != 0 )) && exit_and_write_code $rc

  echo "Accepting licenses..."
  yes | "$sdkmanager" --licenses
  rc=$?
  (( rc != 0 )) && exit_and_write_code $rc

  echo "Creating AVD..."
  "$avdmanager" --clear-cache create avd -n SSCAVD -k "$pkg" -d 1 --force
  rc=$?
  (( rc != 0 )) && exit_and_write_code $rc
fi

[[ -z "$EMULATOR_FLAGS" ]] && EMULATOR_FLAGS=()

EMULATOR_FLAGS+=("-gpu" "swiftshader_indirect")
# fixes adb: failed to install cmd: Can't find service: package

echo "Starting Android emulator..."
if [[ -z "$CI" ]]; then
  "$emulator" @SSCAVD         \
    "${EMULATOR_FLAGS[@]}"    \
    "-gpu swiftshader_indirect" \
    -camera-back none         \
    -no-boot-anim             \
    -no-window                \
    -noaudio                  \
    >/dev/null
else
  "$emulator" @SSCAVD         \
    "${EMULATOR_FLAGS[@]}"    \
    "-gpu swiftshader_indirect" \
    -camera-back none         \
    -no-boot-anim             \
    -no-window                \
    -noaudio                  \
    | grep -v "\[CAMetalLayer nextDrawable\] returning nil because device is nil." \
    2>&1
fi

rc=$?
exit_and_write_code $rc
