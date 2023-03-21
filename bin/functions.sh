#!/usr/bin/env bash

# BSD stat has no version argument or reliable identifier
if stat --help 2>&1 | grep "usage: stat" >/dev/null; then
  stat_format_arg="-f"
  stat_mtime_spec="%m"
  stat_size_spec="%z"
else
  # GNU_STAT
  stat_format_arg="-c"
  stat_mtime_spec="%Y"
  stat_size_spec="%s"
fi

function stat_mtime () {
  stat $stat_format_arg $stat_mtime_spec "$1" 2>/dev/null
}

function stat_size () {
  stat $stat_format_arg $stat_size_spec "$1" 2>/dev/null
}

function quiet () {
  if [ -n "$VERBOSE" ]; then
    echo "$@"
    "$@"
  else
    "$@" > /dev/null 2>&1
  fi

  return $?
}

function die {
  local status=$1
  if (( status != 0 && status != 127 )); then
    for pid in "${pids[@]}"; do
      kill TERM $pid >/dev/null 2>&1
      kill -9 $pid >/dev/null 2>&1
      wait "$pid" 2>/dev/null
    done
    echo "$2 - please report (https://discord.gg/YPV32gKCsH)"
    exit 1
  fi
}

function onsignal () {
  local status=${1:-$?}
  for pid in "${pids[@]}"; do
    kill TERM $pid >/dev/null 2>&1
    kill -9 $pid >/dev/null 2>&1
  done
  exit $status
}

function set_cpu_cores() {
  if [[ -z "$CPU_CORES" ]]; then
    if [[ "Darwin" = "$(uname -s)" ]]; then
      CPU_CORES=$(sysctl -a | grep machdep.cpu.core_count | cut -f2 -d' ')
    else
      CPU_CORES=$(grep 'processor' /proc/cpuinfo | wc -l)
    fi
  fi

  echo $CPU_CORES
}

function host_os() {
  local host=""

  if [[ -n $1 ]]; then
    host=$1
  else
    host="$(uname -s)"
  fi

  if [[ "$host" = "Linux" ]]; then
    if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    echo "WSL is not supported."
    exit 1
    fi
  elif [[ "$host" == *"MINGW64_NT"* ]]; then
    host="Win32"
  elif [[ "$host" == *"MSYS_NT"* ]]; then
    host="Win32"
  fi

  echo "$host"
}

function host_arch() {
  uname -m | sed 's/aarch64/arm64/g'
}
