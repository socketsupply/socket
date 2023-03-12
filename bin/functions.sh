function stat_mtime () {
  if [[ "$(uname -s)" = "Darwin" ]]; then
    if stat --help 2>/dev/null | grep GNU >/dev/null; then
      stat -c %Y "$1" 2>/dev/null
    else
      stat -f %m "$1" 2>/dev/null
    fi
  else
    stat -c %Y "$1" 2>/dev/null
  fi
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
