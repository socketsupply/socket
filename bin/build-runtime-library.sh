#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare clang="${CXX:-${CLANG:-"$(which clang++)"}}"
declare cache_path="$root/build/cache"

source "$root/bin/functions.sh"

declare args=()
declare pids=()
declare force=0

declare arch="$(uname -m)"
declare host_arch=$arch
declare host="$(uname -s)"
declare platform="desktop"
declare host_platform=$platform

if [[ "$host" = "Linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    host="Win32"
  fi
elif [[ "$host" == *"MINGW64_NT"* ]]; then
  host="Win32"
elif [[ "$host" == *"MSYS_NT"* ]]; then
  # Have not confirmed this works as a build host, no gnu find in author's dev
  host="Win32"
fi

declare d=""
if [[ "$host" == "Win32" ]]; then
  # We have to differentiate release and debug for Win32
  if [[ ! -z "$DEBUG" ]]; then
    d="d"
  fi

  if command -v $clang >/dev/null 2>&1; then
    echo > /dev/null
  else
    # POSIX doesn't handle quoted commands
    # Quotes inside variables don't escape spaces, quotes only help on the line we are executing
    # Make a temp link
    clang_tmp=$(mktemp)
    rm $clang_tmp
    ln -s "$clang" $clang_tmp
    clang=$clang_tmp
    # Make tmp.etc look like clang++.etc, makes clang output look correct
    clang=$(echo $clang|sed 's/tmp\./clang++\./')
    mv $clang_tmp $clang
  fi

  declare find_test="$(sh -c 'find --version')"
  if [[ $find_test != *"GNU findutils"* ]]; then
    echo "GNU find not detected. Consider adding %ProgramFiles%\Git\bin\ to PATH."
    echo "NOTE: %ProgramFiles%\Git\usr\bin\ WILL NOT work."
    echo "uname -m: $(uname -s)"
    exit 1
  fi
fi

declare objects=()
declare sources=(
  $(find "$root"/src/app/*.cc)
  $(find "$root"/src/core/*.cc)
  $(find "$root"/src/ipc/*.cc)
)

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
    pass_force="$arg"
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
    elif [[ "$1" = "android" ]]; then
      platform="$1";
      export TARGET_OS_ANDROID=1      
    else
      platform="$1";
    fi
    shift
    continue
  fi

  args+=("$arg")
done

if [[ "$platform" = "android" ]]; then
  source "$root/bin/android-functions.sh"

  if [[ ! -z $DEPS_ERROR ]]; then
    echo "Android dependencies not satisfied."
    exit 1
  fi

  clang=$(android_clang $ANDROID_HOME $NDK_VERSION $host $host_arch $arch "++")
elif [[ "$host" = "Darwin" ]]; then
  cflags+=("-ObjC++")
  sources+=("$root/src/window/apple.mm")
  if (( TARGET_OS_IPHONE)); then
    clang="xcrun -sdk iphoneos $clang"
  elif (( TARGET_IPHONE_SIMULATOR )); then
    clang="xcrun -sdk iphonesimulator $clang"
  else
    sources+=("$root/src/process/unix.cc")
  fi
elif [[ "$host" = "Linux" ]]; then
  sources+=("$root/src/window/linux.cc")
  sources+=("$root/src/process/unix.cc")
elif [[ "$host" = "Win32" ]]; then
  sources+=("$root/src/window/win.cc")
  sources+=("$root/src/process/win.cc")
fi

declare cflags=($("$root/bin/cflags.sh"))
declare ldflags=($("$root/bin/ldflags.sh"))

if [[ "$platform" = "android" ]]; then
  cflags+=("${android_includes[@]}")
fi

declare output_directory="$root/build/$arch-$platform"
mkdir -p "$output_directory"

cd "$(dirname "$output_directory")"

echo "# building runtime static libary ($arch-$platform)"
for source in "${sources[@]}"; do
  declare src_directory="$root/src"
  declare object="${source/.cc/$d.o}"
  declare object="${object/$src_directory/$output_directory}"
  objects+=("$object")
done

function main () {
  trap onsignal INT TERM
  local i=0
  local max_concurrency=8

  for source in "${sources[@]}"; do
    if (( i++ > max_concurrency )); then
      for pid in "${pids[@]}"; do
        wait "$pid" 2>/dev/null
      done
      i=0
    fi

    {
      declare src_directory="$root/src"
      declare object="${source/.cc/$d.o}"
      declare object="${object/$src_directory/$output_directory}"
      if (( force )) || ! test -f "$object" || (( $(stat_mtime "$source") > $(stat_mtime "$object") )); then
        mkdir -p "$(dirname "$object")"
        echo "# compiling object ($arch-$platform) $(basename "$source")"
        quiet $clang "${cflags[@]}" -c "$source" -o "$object" || onsignal
        echo "ok - built ${source/$src_directory\//} -> ${object/$output_directory\//} ($arch-$platform)"
      fi
    } & pids+=($!)
  done

  for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null
  done

  declare static_library="$root/build/$arch-$platform/lib$d/libsocket-runtime$d.a"
  mkdir -p "$(dirname "$static_library")"
  declare ar="ar"

  if [[ "$platform" = "android" ]]; then
    ar=$(android_ar $ANDROID_HOME $NDK_VERSION $host $host_arch)
  elif [[ "$host" = "Win32" ]]; then
    ar="llvm-ar"
  fi

  local build_static=0
  local static_library_mtime=$(stat_mtime "$static_library")
  for source in "${objects[@]}"; do
    if ! test -f $source; then
      echo "$source not built.."
      exit 1
    fi
    if (( force )) || ! test -f "$static_library" || (( $(stat_mtime "$source") > $static_library_mtime )); then
      build_static=1
      break
    fi
  done

  # echo $ar crs "$static_library" "${objects[@]}"
  if (( build_static )); then
    $ar crs "$static_library" "${objects[@]}"

    if [ -f $static_library ]; then
      echo "ok - built static library ($arch-$platform): $(basename "$static_library")"
    else
      echo "failed to build $static_library"
    fi
  else
    if [ -f $static_library ]; then
      echo "ok - using cached static library ($arch-$platform): $(basename "$static_library")"
    else
      echo "static library doesn't exist after cache check passed: ($arch-$platform): $(basename "$static_library")"
    fi
  fi
}

main "${args[@]}"
exit $?
