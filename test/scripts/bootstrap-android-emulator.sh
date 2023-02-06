#!/usr/bin/env bash

ANDROID_HOME="${ANDROID_HOME:-$HOME/.android/sdk}"
emulator_flags=()
emulator="$(which emulator 2>/dev/null)"

if [ -z "$emulator" ]; then
  emulator="$ANDROID_HOME/emulator/emulator"
fi

emulator_flags+=(
  -gpu swiftshader_indirect
  -camera-back none
  -no-boot-anim
  -no-window
  -noaudio
)

emulator @SSCAVD "${emulator_flags[@]}" >/dev/null
