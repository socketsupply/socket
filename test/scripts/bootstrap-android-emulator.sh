#!/usr/bin/env bash

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

"$ANDROID_HOME/emulator/emulator" @SSCAVD "${emulator_flags[@]}" >/dev/null
