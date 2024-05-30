#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare clang="${CXX:-"$CLANG"}"
declare cache_path="$root/build/cache"

source "$root/bin/functions.sh"
export CPU_CORES=$(set_cpu_cores)

declare args=()
declare pids=()
declare force=0

declare arch="$(host_arch)"
declare host_arch=$arch
declare host=$(host_os)
declare platform="desktop"

declare d=""
if [[ "$host" == "Win32" ]]; then
  # We have to differentiate release and debug for Win32
  if [[ -n "$DEBUG" ]]; then
    d="d"
  fi

  if command -v "$clang" >/dev/null 2>&1; then
    write_log "d" "Found clang++ on path"
  elif [ -f "$clang" ]; then
    write_log "d" "Full path clang++ exists: $clang"
  elif [ -n "$CXX" ]; then
    write_log "v" "no clang on path, making tmp link."
    # POSIX doesn't handle quoted commands
    # Quotes inside variables don't escape spaces, quotes only help on the line we are executing
    # Make a temp link
    clang_tmp=$(mktemp)
    rm $clang_tmp
    ln -s "$clang" $clang_tmp
    clang=$clang_tmp
    # Make tmp.etc look like clang++.etc, makes clang output look correct
    clang=$(echo $CXX|sed 's/tmp\./clang++\./')
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
      [[ -z "$arch" ]] && arch="x86_64"
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

  # Don't rebuild if header mtimes are newer than .o files - Be sure to manually delete affected assets as required
  if [[ "$arg" == "--ignore-header-mtimes" ]]; then
    ignore_header_mtimes=1; continue
  fi

  args+=("$arg")
done

declare objects=()
declare sources=(
  $(find "$root"/src/app/*.cc)
  $(find "$root"/src/core/*.cc)
  $(find "$root"/src/core/modules/*.cc)
  $(find "$root"/src/extension/*.cc)
  $(find "$root"/src/ipc/*.cc)
  $(find "$root"/src/platform/*.cc)
  $(find "$root"/src/serviceworker/*.cc)
  "$root/build/llama/common/common.cpp"
  "$root/build/llama/common/sampling.cpp"
  "$root/build/llama/common/json-schema-to-grammar.cpp"
  "$root/build/llama/common/grammar-parser.cpp"
  "$root/build/llama/llama.cpp"
  "$root/src/window/manager.cc"
  "$root/src/window/dialog.cc"
  "$root/src/window/hotkey.cc"
)

declare test_headers=()
declare cflags

if [[ "$platform" = "android" ]]; then
  source "$root/bin/android-functions.sh"
  android_fte

  if [[ -n $ANDROID_DEPS_ERROR ]]; then
    echo >&2 "not ok - Android dependencies not satisfied."
    exit 1
  fi

  clang="$(android_clang "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch" "++")"
  clang_target="$(android_clang_target "$arch")"
  sources+=("$root/src/core/process/unix.cc")
  sources+=($(find "$root/src/platform/android"/*.cc))
  sources+=("$root/src/window/android.cc")
elif [[ "$host" = "Darwin" ]]; then
  sources+=("$root/src/window/apple.mm")

  if (( TARGET_OS_IPHONE)); then
    clang="xcrun -sdk iphoneos "$clang""
  elif (( TARGET_IPHONE_SIMULATOR )); then
    clang="xcrun -sdk iphonesimulator "$clang""
  else
    sources+=("$root/src/core/process/unix.cc")
  fi
elif [[ "$host" = "Linux" ]]; then
  sources+=("$root/src/window/linux.cc")
  sources+=("$root/src/core/process/unix.cc")
elif [[ "$host" = "Win32" ]]; then
  sources+=("$root/src/window/win.cc")
  sources+=("$root/src/core/process/win.cc")
fi

cflags+=($(ARCH="$arch" "$root/bin/cflags.sh"))

if [[ "$platform" = "android" ]]; then
  cflags+=("$clang_target ${android_includes[*]}")
fi

declare output_directory="$root/build/$arch-$platform"
mkdir -p "$output_directory"

cd "$(dirname "$output_directory")"

echo "# building runtime static libary ($arch-$platform)"
for source in "${sources[@]}"; do
  declare src_directory="$root/src"

  declare object="${source/.cc/$d.o}"
  object="${object/.cpp/$d.o}"

  declare build_dir="$root/build"

  if [[ "$object" =~ ^"$src_directory" ]]; then
    object="${object/$src_directory/$output_directory}"
  else
    object="${object/$build_dir/$output_directory}"
  fi

  objects+=("$object")
done

objects+=("$output_directory/llama/build-info.o")

if [[ -z "$ignore_header_mtimes" ]]; then
  test_headers+="$(find "$root/src"/core/*.hh)"
fi

function generate_llama_build_info () {
  build_number="0"
  build_commit="unknown"
  build_compiler="unknown"
  build_target="unknown"

  if out=$(git rev-list --count HEAD); then
    # git is broken on WSL so we need to strip extra newlines
    build_number=$(printf '%s' "$out" | tr -d '\n')
  fi

  if out=$(git rev-parse --short HEAD); then
    build_commit=$(printf '%s' "$out" | tr -d '\n')
  fi

  if out=$($clang --version | head -1); then
    build_compiler=$out
  fi

  if out=$($clang -dumpmachine); then
    build_target=$out
  fi

  echo "# generating llama build info"
  declare source="$output_directory/llama/build-info.cpp"

  cat > $source << LLAMA_BUILD_INFO
    int LLAMA_BUILD_NUMBER = $build_number;
    char const *LLAMA_COMMIT = "$build_commit";
    char const *LLAMA_COMPILER = "$build_compiler";
    char const *LLAMA_BUILD_TARGET = "$build_target";
LLAMA_BUILD_INFO

  quiet $clang "${cflags[@]}" -c $source -o ${source/cpp/o} || onsignal
}

function main () {
  trap onsignal INT TERM
  local i=0
  local max_concurrency=$CPU_CORES

  local newest_mtime=0

  mkdir -p "$output_directory/include"
  cp -rf "$root/include"/* "$output_directory/include"
  rm -f "$output_directory/include/socket/_user-config-bytes.hh"

  generate_llama_build_info

  for source in "${sources[@]}"; do
    if (( ${#pids[@]} > max_concurrency )); then
      wait "${pids[0]}" 2>/dev/null
      pids=("${pids[@]:1}")
    fi

    {
      declare src_directory="$root/src"

      declare object="${source/.cc/$d.o}"
      object="${object/.cpp/$d.o}"

      declare header="${source/.cc/.hh}"
      header="${header/.cpp/.h}"

      declare build_dir="$root/build"

      if [[ "$object" =~ ^"$src_directory" ]]; then
        object="${object/$src_directory/$output_directory}"
      else
        object="${object/$build_dir/$output_directory}"
      fi

      if (( force )) ||
        ! test -f "$object" ||
        (( newest_mtime > $(stat_mtime "$object") )) ||
        (( $(stat_mtime "$source") > $(stat_mtime "$object") )) ||
        (( $(stat_mtime "$header") > $(stat_mtime "$source") ));
      then
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

  declare base_lib="libsocket-runtime";
  declare static_library="$root/build/$arch-$platform/lib$d/$base_lib$d.a"
  mkdir -p "$(dirname "$static_library")"
  declare ar="ar"

  if [[ "$platform" = "android" ]]; then
    ar="$(android_ar "$ANDROID_HOME" "$NDK_VERSION" "$host" "$host_arch")"
  elif [[ "$host" = "Win32" ]]; then
    ar="llvm-ar"
  fi

  local build_static=0
  local static_library_mtime=$(stat_mtime "$static_library")
  for source in "${objects[@]}"; do
    if ! test -f "$source"; then
      echo "$source not built.."
      exit 1
    fi
    if (( force )) || ! test -f "$static_library" || (( $(stat_mtime "$source") > "$static_library_mtime" )); then
      build_static=1
      break
    fi
  done

  if (( build_static )); then
    $ar crs "$static_library" "${objects[@]}"

    if [ -f "$static_library" ]; then
      echo "ok - built static library ($arch-$platform): $(basename "$static_library")"
    else
      echo "failed to build $static_library"
      exit 1
    fi
  else
    if [ -f "$static_library" ]; then
      echo "ok - using cached static library ($arch-$platform): $(basename "$static_library")"
    else
      echo "static library doesn't exist after cache check passed: ($arch-$platform): $(basename "$static_library")"
      exit 1
    fi
  fi

  if [[ "$host" = "Linux" ]] && [[ "$platform" = "desktop" ]]; then
    "$root/bin/generate-socket-runtime-pkg-config.sh"
  fi

  if [[ "$platform" == "android" ]]; then
    # This is a sanity check to confirm that the static_library is > 8 bytes
    # If an empty ${objects[@]} is provided to ar, it will still spit out a header without an error code.
    # therefore check the output size
    # This error condition should only occur after a code change
    lib_size=$(stat_size "$static_library")
    if (( lib_size < $(android_min_expected_static_lib_size "$base_lib") )); then
      echo >&2 "not ok -  $static_library size looks wrong: $lib_size, renaming as .bad"
      mv "$static_library" "$static_library.bad"
      exit 1
    fi
  fi
}

main "${args[@]}"
exit $?
