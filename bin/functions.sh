#!/usr/bin/env bash

declare SSC_ENV_FILENAME=".ssc.env"

export SSC_ENV_FILENAME

# BSD stat has no version argument or reliable identifier
if stat --help 2>&1 | grep "usage: stat" >/dev/null; then
  stat_format_arg="-f"
  stat_mtime_spec="%m"
  stat_size_spec="%z"
  _sha512sum="shasum -a512"
else
  # GNU_STAT
  stat_format_arg="-c"
  stat_mtime_spec="%Y"
  stat_size_spec="%s"
  _sha512sum="sha512sum"
fi

function stat_mtime () {
  stat $stat_format_arg $stat_mtime_spec "$1" 2>/dev/null
}

function stat_size () {
  stat $stat_format_arg $stat_size_spec "$1" 2>/dev/null
}

function sha512sum() {
  # Can't figure out a better way of escaping $_sha512sum for use in a call than using sh -c
  sh -c "$_sha512sum $1|cut -d' ' -f1"
}

function escape_path() {
  local r=$1
  local host="$(host_os)"
  if [[ "$host" == "Win32" ]]; then
    r=${r//\\/\\\\}
  fi
  echo "$r"
}

function unix_path() {
  local p="$(escape_path "$1")"
  local host="$(host_os)"
  if [[ "$host" == "Win32" ]]; then
    p="$(cygpath -u "$p")"
    # cygpath doesn't escape spaces
    echo "${p//\ /\\ }"
    return
  fi
  echo "$p"
}

function native_path() {
  local host="$(host_os)"
  if [[ "$host" == "Win32" ]]; then
    local p="$(cygpath -w "$1")"
    if [[ "$p" == *"\\ "* ]]; then
      # string contains escaped space, quote it and de-escape
      p="\"${p//\\ /\ }\""
    fi
    echo "$p"
    return
  fi
  echo "$1"
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
    write_log "h" "$2 - please report (https://discord.gg/YPV32gKCsH)"
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
    write_log "h" "WSL is not supported."
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

function build_env_data() {
  echo "ANDROID_HOME=\"$(escape_path "$ANDROID_HOME")\""
  echo "JAVA_HOME=\"$(escape_path "$JAVA_HOME")\""
  echo "ANDROID_SDK_MANAGER=\"$(escape_path "$ANDROID_SDK_MANAGER")\""
  echo "GRADLE_HOME=\"$(escape_path "$GRADLE_HOME")\""
  # Should not use these for general calls
  echo "ANDROID_SDK_MANAGER_JAVA_OPTS=\"-XX:+IgnoreUnrecognizedVMOptions --add-modules java.se.ee\""
  echo "ANDROID_SDK_MANAGER_ACCEPT_LICENSES=\"$ANDROID_SDK_MANAGER_ACCEPT_LICENSES\""
}

function read_env_data() {
  if [[ -f "$SSC_ENV_FILENAME" ]]; then
    source "$root/$SSC_ENV_FILENAME"
  fi
}

function write_env_data() {
  # Maintain mtime on $SSC_ENV_FILENAME, only update if changed
  local temp=$(mktemp)
  local new_hash
  local old_hash
  build_env_data > "$temp"

  if [[ ! -f "$root/$SSC_ENV_FILENAME" ]]; then
    mv "$temp" "$root/$SSC_ENV_FILENAME"
  else
    old_hash=$(sha512sum "$root/$SSC_ENV_FILENAME")
    new_hash=$(sha512sum "$temp")

    if [[ "$old_hash" != "$new_hash" ]]; then
      mv "$temp" "$root/$SSC_ENV_FILENAME"
    else
      rm -f "$temp"
    fi
  fi
}

function prompt() {
  write_log "h" "$1"
  local return=$2
  local input
  read -rp '> ' input
  # effectively stores $input in $2 by reference, rather than using echo to return which would prevent echo "$1" going to stdout
  eval "$return=\"$input\""
  write_log "f" "input: $input"
}

function prompt_yn() {
  local r
  local return=$2
  prompt "$1 [y/N]" r

  if [[ "$(lower "$r")" == "y" ]]; then
    return 0
  fi
  return 1
}

function prompt_new_path() {
  local text="$1"
  local return=$3
  local exists_message=$4
  local input=""
  local unix_input=""

  if [ -n "$2" ]; then
    write_log "h" "$text"
    if prompt_yn "Use suggested default path: ""$2""?"; then
      input="$2"
    fi
  fi

  while true; do
    if [ -z "$input" ]; then
      prompt "$text (Press Enter to go back)" input
    fi
    # remove any quote characters
    input=${input//\"/}
    unix_input="$(unix_path "$input")"
    if [ -e "$unix_input" ]; then
      if [ "$exists_message" == "!CAN_EXIST!" ]; then
        input="$(native_path "$(abs_path "$unix_input")")"
        eval "$return=$(escape_path "$input")"
        return 0
      fi

      unix_input="$(abs_path "$unix_input")"
      write_log "h" "\"$input\" already exists, please choose a new path."
      if [ -n "$exists_message" ]; then
        write_log "h" "$exists_message"
      fi
      input=""
    elif [ -z "$input" ]; then
      if prompt_yn "Cancel entering new path?"; then
        return
      fi
    else
      write_log "h" "Create: $unix_input"
      if ! mkdir -p "$unix_input"; then
        write_log "h" "Create $unix_input failed."
        input=""
      else
        input="$(native_path "$(abs_path "$unix_input")")"
        eval "$return=$(escape_path "$input")"
        return 0
      fi
    fi
  done
}

function abs_path() {
  local test="$1"
  local basename=""
  if [ -f "$1" ]; then
    test="$(dirname "$1")"
    basename="/$(basename "$1")"
  elif [ ! -e "$1" ]; then
    return 1
  fi

  local p="$(sh -c "cd '$test'; pwd")$basename"
  # mingw sh returns incorrect escape slash if path contains spaces, swap / for \
  p="${p///\ /\\ }"
  echo "$p"
}

download_to_tmp() {
  local uri=$1
  local tmp="$(mktemp -d)"
  local output=$tmp/"$(basename "$uri")"
  local http_code

  if [ -n "$SSC_ANDROID_REPO" ]; then
    echo >&2 cp "$SSC_ANDROID_REPO/$(basename "$uri")" "$output"
    cp "$SSC_ANDROID_REPO/$(basename "$uri")" "$output" || return $?
  else
    http_code=$(curl -L --write-out '%{http_code}' "$uri" --output "$output")
    # DONT COMMIT
    cp "$output" ..
    if  [ "$http_code" != "200" ] ; then
      echo "$http_code"
      rm -rf "$tmp"
      return 1
    fi
  fi
  echo "$output"
}

function unpack() {
  local archive=$1
  local dest=$2
  local command=""

  if [[ "$archive" == *".tar.gz" ]]; then
    command="tar -xf"
  elif [[ "$archive" == *".gz" ]]; then
    command="gzip -d";
  elif [[ "$archive" == *".bz2" ]]; then
    command="bzip2 -d"
  elif [[ "$archive" == *".zip" ]]; then
    command="unzip"
  fi

  if ! cd "$dest"; then
    return $?
  fi

  $command "$archive"
  return 0
}

function get_top_level_archive_dir() {
  local archive=$1
  local head=""

  if [[ "$archive" == *".tar.gz" ]] || [[ "$archive" == *".tgz" ]]; then
    tar -tf "$archive" | while read -r line; do
      for head in $(echo "$line" | tr "/" "\n"); do
        if [[ "$head" == "." ]]; then
          : continue
        else
          echo "$head"
          return
        fi
      done
    done
    return $?
  elif [[ "$archive" == *".gz" ]] || [[ "$archive" == *".bz2" ]]; then
    "$(basename "${archive%.*}")"
  elif [[ "$archive" == *".bz2" ]]; then
    "$(basename "${archive%.*}")"
  elif [[ "$archive" == *".zip" ]]; then
    head=$(unzip -Z1 "$archive" | head -n1)
    echo "${head//\//}" # remove trailing slash
  fi

  return $?
}

function lower() {
  echo "$1"|tr '[:upper:]' '[:lower:]'
}

function version { 
  echo "$@" | awk -F. '{ printf("%d%03d%03d%03d\n", $1,$2,$3,$4); }'; 
}

# Logging
_loghome=$HOME
if [ -n "$LOCALAPPDATA" ]; then
  _loghome="$LOCALAPPDATA"
fi

logfile="$_loghome/socket_install_sh.log"

function write_log() {
  local type=$1
  local message=$2
  if [[ -n "$DEBUG" ]] && ( [[ "$type" == "d" ]] || [[ "$type" == "v" ]] ); then
    wh="1"
  elif [[ -n "$VERBOSE" ]] && [[ "$type" == "v" ]]; then
    wh="1"
  elif [[ "$type" == "h" ]]; then
    wh="1"
  elif [[ "$type" == "f" ]]; then
    unset wh
  fi

  if [[ -n "$wh" ]]; then
    echo "$message"
  fi

  # Write-LogFile $message
  write_log_file "$message"
}

function write_log_file() {
  echo "$1" >> "$logfile"
}
