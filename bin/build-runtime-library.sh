#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare clang="${CXX:-${CLANG:-"$(which clang++)"}}"
declare cache_path="$root/build/cache"

declare args=()
declare pids=()
declare force=0

declare objects=()
declare sources=(
  $(find "$root"/src/app/*.cc)
  $(find "$root"/src/core/*.cc)
  $(find "$root"/src/ipc/*.cc)
)

declare arch="$(uname -m)"
declare platform="desktop"

if (( TARGET_OS_IPHONE )); then
  arch="arm64"
  platform="iPhoneOS"
elif (( TARGET_IPHONE_SIMULATOR )); then
  arch="x86_64"
  platform="iPhoneSimulator"
fi

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
   force=1; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    if [[ "$1" = "ios" ]] || [[ "$1" = "iPhoneOS" ]] || [[ "$1" = "iphoneos" ]]; then
      arch="arm64"
      platform="iPhoneOS";
      export TARGET_OS_IPHONE=1
    elif [[ "$1" = "ios-simulator" ]] || [[ "$1" = "iPhoneSimulator" ]] || [[ "$1" = "iphonesimulator" ]]; then
      arch="x86_64"
      platform="iPhoneSimulator";
      export TARGET_IPHONE_SIMULATOR=1
    else
      platform="$1";
    fi
    shift
    continue
  fi

  args+=("$arg")
done

if [[ "$(uname -s)" = "Darwin" ]]; then
  cflags+=("-ObjC++")
  sources+=("$root/src/window/apple.mm")
  if (( TARGET_OS_IPHONE)); then
    clang="xcrun -sdk iphoneos $clang"
  elif (( TARGET_IPHONE_SIMULATOR )); then
    clang="xcrun -sdk iphonesimulator $clang"
  else
    sources+=("$root/src/process/unix.cc")
  fi
elif [[ "$(uname -s)" = "Linux" ]]; then
  sources+=("$root/src/window/linux.cc")
  sources+=("$root/src/process/unix.cc")
fi

declare cflags=($("$root/bin/cflags.sh"))
declare ldflags=($("$root/bin/ldflags.sh"))

declare output_directory="$root/build/$arch-$platform"
mkdir -p "$output_directory"

cd "$(dirname "$output_directory")"

echo "# building runtime static libary ($arch-$platform)"
for source in "${sources[@]}"; do
  declare src_directory="$root/src"
  declare object="${source/.cc/.o}"
  declare object="${object/$src_directory/$output_directory}"
  objects+=("$object")
done

function onsignal () {
  local status=${1:-$?}
  for pid in "${pids[@]}"; do
    kill TERM $pid >/dev/null 2>&1
    kill -9 $pid >/dev/null 2>&1
  done
  exit $status
}

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

function main () {
  trap onsignal INT TERM
  let local i=0
  let local max_concurrency=8

  for source in "${sources[@]}"; do
    if (( i++ > max_concurrency )); then
      for pid in "${pids[@]}"; do
        wait "$pid" 2>/dev/null
      done
      i=0
    fi

    {
      declare src_directory="$root/src"
      declare object="${source/.cc/.o}"
      declare object="${object/$src_directory/$output_directory}"
      if (( force )) || ! test -f "$object" || (( $(stat_mtime "$source") > $(stat_mtime "$object") )); then
        mkdir -p "$(dirname "$object")"
        echo "# compiling object ($arch-$platform) $(basename "$source")"
        # echo $clang "${cflags[@]}" "${ldflags[@]}" -c "$source" -o "$object"
        $clang "${cflags[@]}" -c "$source" -o "$object" || onsignal
        echo "ok - built ${source/$src_directory\//} -> ${object/$output_directory\//} ($arch-$platform)"
      fi
    } & pids+=($!)
  done

  for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null
  done

  declare static_library="$root/build/$arch-$platform/lib/libsocket-runtime.a"
  mkdir -p "$(dirname "$static_library")"
  rm -rf "$static_library"
  # echo ar crs "$static_library" "${objects[@]}"
  ar crs "$static_library" "${objects[@]}"
  echo "ok - built static library ($arch-$platform): $(basename "$static_library")"
}

main "${args[@]}"
exit $?
