#!/bin/bash

# adapted from: https://gitlab.com/-/snippets/1715481

##
# Usage:
# . vc-env.sh
##

declare VCVARSALL_BAT_SCRIPT=""
declare PLATFORM="x64"

declare MSENV_BATCH_NAME=""
declare MSENV_BATCH=""
declare MSENV=""

if [ -z "$MSENV_IS_SET" ]; then
  if [[ "$1" == "" ]]; then
    echo " info: Assuming $PLATFORM as platform"
    echo " info: Specify the platform as the first argument for this script to override"
  else
    PLATFORM="$1"
  fi

  if which vswhere > /dev/null 2>&1; then
    VSWHERE_INSTALLPATH="$(vswhere -property 'installationPath' -requires 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64' -version '[15.0,16.0\)')"
    VSWHERE_INSTALLPATH_BASH="$(echo $VSWHERE_INSTALLPATH | sed -e 's/^\([A-Za-z]\)\:\\\(.*\)/\/\1\/\2/;s/\\/\//g;')"

    VCVARSALL_BAT_SCRIPT="$VSWHERE_INSTALLPATH_BASH/VC/Auxiliary/Build/vcvarsall.bat"
  elif [ -z "$VCVARSALL_BAT_SCRIPT" ] || [ ! -e "$VCVARSALL_BAT_SCRIPT" ]; then
    echo " info: Looking for 'vcvarsall.bat'"

    VCVARSALL_BAT_SCRIPT="/c/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools/VC/Auxiliary/Build/vcvarsall.bat"
    if [ ! -e "$VCVARSALL_BAT_SCRIPT" ]; then
      VCVARSALL_BAT_SCRIPT="/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Auxiliary/Build/vcvarsall.bat"
      if [ ! -e "$VCVARSALL_BAT_SCRIPT" ]; then
        VCVARSALL_BAT_SCRIPT="/c/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Auxiliary/Build/vcvarsall.bat"
        if [ ! -e "$VCVARSALL_BAT_SCRIPT" ]; then
          VCVARSALL_BAT_SCRIPT="/c/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Auxiliary/Build/vcvarsall.bat"
        fi
      fi
    fi
  fi

  if [ ! -e "$VCVARSALL_BAT_SCRIPT" ]; then
    echo "error: Failed to locate vcvarsall.bat" >&2
  else
    MSENV_BATCH_NAME="__print_ms_env_$$.bat"
    MSENV_BATCH="/tmp/$MSENV_BATCH_NAME"
    MSENV="/tmp/__ms_env_$$"

    echo "@echo off" > "$MSENV_BATCH"
    echo "call \"$(echo $VCVARSALL_BAT_SCRIPT | sed -e 's/^\/\([A-Za-z]\)\(.*\)/\1\:\2/;s/\//\\/g')\" $PLATFORM" >> "$MSENV_BATCH"
    echo "set" >> "$MSENV_BATCH"

    cmd "/C %TEMP%\\$MSENV_BATCH_NAME" > "$MSENV.tmp"

    grep -E '^PATH=' "$MSENV.tmp" | sed -e 's/\(.*\)=\(.*\)/export \1="\2"/g;s/\([a-zA-Z]\):[\\\/]/\/\1\//g;s/\\/\//g;s/;\//:\//g' > "$MSENV"
    grep -E '^(INCLUDE|LIB|LIBPATH)=' "$MSENV.tmp" | sed -e 's/\(.*\)=\(.*\)/export \1="\2"/g' >> "$MSENV"

    if which clang++ > /dev/null 2>&1; then
      echo " warn: clang++ is already in your PATH. Undefined behavior may occur"
    fi

    if which clang > /dev/null 2>&1; then
      echo " warn: clang is already in your PATH. Undefined behavior may occur"
    fi

    unset CXX
    unset CC

    source "$MSENV"

    echo " info: clang++=$(which clang++)"
    echo " info: Clang=$(which clang)"

    rm -f "$MSENV_BATCH"
    rm -f "$MSENV.tmp"
    rm -f "$MSENV"

    MSENV_IS_SET=1
    export MSENV_IS_SET

    export CXX
    export CC
  fi
fi
