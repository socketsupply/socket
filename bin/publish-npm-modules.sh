#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

source "$root/bin/android-functions.sh"
source "$root/bin/functions.sh"

declare archs=($(host_arch))
declare platform="$(uname -s | tr '[[:upper:]]' '[[:lower:]]')"

declare args=()
declare dry_run=0
declare only_platforms=0
declare only_top_level=0
declare no_rebuild=0
declare remove_socket_home=1
declare do_global_link=0

function _publish () {
  if (( !dry_run && !do_global_link )); then
    npm publish "${args[@]}" || exit $?
  elif (( !do_global_link )); then
    # echo "# npm publish ${args[@]}"
    npm pack "${args[@]}" || exit $?
  fi
}

if [[ "$platform" = "linux" ]]; then
  if [ -n "$WSL_DISTRO_NAME" ] || uname -r | grep 'Microsoft'; then
    platform="Win32"
  fi
elif [[ "$(uname -s)" == *"MINGW64_NT"* ]]; then
  platform="Win32"
elif [[ "$(uname -s)" == *"MSYS_NT"* ]]; then
  platform="Win32"
fi

declare SOCKET_HOME="$root/build/npm/$platform"
declare PREFIX="$SOCKET_HOME"

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--dry-run" ]] || [[ "$arg" = "-n" ]]; then
    dry_run=1
    continue
  fi

  if [[ "$arg" = "--only-platforms" ]]; then
    only_platforms=1
    continue
  fi

  if [[ "$arg" = "--only-top-level" ]]; then
    only_top_level=1
    continue
  fi

  if [[ "$arg" = "--no-remove-socket-home" ]]; then
    remove_socket_home=0
    continue
  fi

  if [[ "$arg" = "--no-rebuild" ]]; then
    no_rebuild=1
    continue
  fi

  if [[ "$arg" = "--link" ]]; then
    do_global_link=1
    continue
  fi

  args+=("$arg")
done

if (( remove_socket_home )); then
  rm -rf "$SOCKET_HOME"
fi

mkdir -p "$SOCKET_HOME"

export SOCKET_HOME
export PREFIX

if (( !only_top_level && !no_rebuild )) ; then
  "$root/bin/install.sh" || exit $?
fi

if (( do_global_link && !dry_run )); then
  dry_run=1
fi

declare ABORT_ERRORS=0

# Confirm that build SSC matches current commit

if (( ! only_top_level )); then
  if command -v ssc >/dev/null 2>&1; then
    REPO_VERSION="$(cat "$root/VERSION.txt") ($(git rev-parse --short=8 HEAD))"
    BUILD_VERSION=$("$SOCKET_HOME"/bin/ssc --version)

    if [[ "$REPO_VERSION" != "$BUILD_VERSION" ]]; then
      echo "Repo $REPO_VERSION and $SOCKET_HOME/bin/ssc $BUILD_VERSION don't match."
      ABORT_ERRORS=1
    fi
  fi
fi

declare android_abis=()


if (( only_top_level )); then
  : # android libs don't need to be built for top level
elif [[ "arm64" == "$(host_arch)" ]] && [[ "linux" == "$platform" ]]; then
  echo "warn - Android not supported on $platform-"$(uname -m)""
else
  android_abis+=($(android_supported_abis))
fi

if (( ! do_global_link )); then
  for abi in "${android_abis[@]}"; do
    lib_path="$SOCKET_HOME/lib/$abi-android"
    if [[ ! -f "$lib_path/libuv.a" ]]; then
      ABORT_ERRORS=1
      echo >&2 "not ok - $lib_path/libuv.a missing - check build process."
    fi

    if [[ ! -f "$lib_path/libsocket-runtime.a" ]]; then
      ABORT_ERRORS=1
      echo >&2 "not ok - $lib_path/libsocket-runtime.a missing - check build process."
    fi
    export ABORT_ERRORS
  done

  if (( ABORT_ERRORS )); then
    echo >&2 "not ok - Refusing to publish due to errors."
    exit 1
  fi
fi

mkdir -p "$SOCKET_HOME/packages/@socketsupply"

if (( !only_platforms || only_top_level )); then
  declare package="@socketsupply/socket"
  cp -rf "$root/npm/packages/@socketsupply/socket" "$SOCKET_HOME/packages/$package"
  mkdir -p "$SOCKET_HOME/packages/$package/bin"
  cp -rf "$root/npm/bin/ssc.js" "$SOCKET_HOME/packages/$package/bin/ssc.js"
  cp -f "$root/LICENSE.txt" "$SOCKET_HOME/packages/$package"
  cp -f "$root/README.md" "$SOCKET_HOME/packages/$package/README-RUNTIME.md"
  cp -rf "$root/api"/* "$SOCKET_HOME/packages/$package"
fi

if (( !only_top_level )); then
  for arch in "${archs[@]}"; do
    declare package="@socketsupply/socket-$platform-${arch/x86_64/x64}"
    cp -rf "$root/npm/packages/$package" "$SOCKET_HOME/packages/$package"

    mkdir -p "$SOCKET_HOME/packages/$package/uv"
    mkdir -p "$SOCKET_HOME/packages/$package/bin"
    mkdir -p "$SOCKET_HOME/packages/$package/src"
    mkdir -p "$SOCKET_HOME/packages/$package/include"
    mkdir -p "$SOCKET_HOME/packages/$package/lib"
    mkdir -p "$SOCKET_HOME/packages/$package/objects"

    cp -rf "$root/npm/bin"/* "$SOCKET_HOME/packages/$package/bin"
    cp -rf "$root/npm/src"/* "$SOCKET_HOME/packages/$package/src"
    cp -f "$root/LICENSE.txt" "$SOCKET_HOME/packages/$package"
    cp -f "$root/README.md" "$SOCKET_HOME/packages/$package"

    cp -rf "$SOCKET_HOME/uv"/* "$SOCKET_HOME/packages/$package/uv"
    cp -rf "$SOCKET_HOME/bin"/* "$SOCKET_HOME/packages/$package/bin"
    cp -rf "$SOCKET_HOME/src"/* "$SOCKET_HOME/packages/$package/src"
    cp -rf "$SOCKET_HOME/include"/* "$SOCKET_HOME/packages/$package/include"

    # don't copy debug files, too large
    rm -rf $SOCKET_HOME/lib/*-android/objs-debug
    cp -rf $SOCKET_HOME/lib/*-android "$SOCKET_HOME/packages/$package/lib"

    cp -rf "$SOCKET_HOME/lib/"$arch-* "$SOCKET_HOME/packages/$package/lib"
    cp -rf "$SOCKET_HOME/objects/"$arch-* "$SOCKET_HOME/packages/$package/objects"

    ## Install x86_64-iPhoneSimulator files for arm64 too
    if [ "$platform" = "darwin" ]; then
      if [ "$(uname -m)" == "arm64" ]; then
        cp -rf "$SOCKET_HOME/lib/x86_64-iPhoneSimulator" "$SOCKET_HOME/packages/$package/lib"
        cp -rf "$SOCKET_HOME/objects/x86_64-iPhoneSimulator" "$SOCKET_HOME/packages/$package/objects"
      fi
    fi

    if [ "$platform" = "Win32" ]; then
      cp -rap "$SOCKET_HOME/bin"/.vs* "$SOCKET_HOME/packages/$package/bin"
    fi

    cd "$SOCKET_HOME/packages/$package" || exit $?
    echo "# in directory: '$SOCKET_HOME/packages/$package'"

    _publish

    if (( do_global_link )); then
      npm link --no-fund --no-audit --offline
    fi
  done
fi

if (( !only_platforms || only_top_level )); then
  cd "$SOCKET_HOME/packages/@socketsupply/socket" || exit $?
  echo "# in directory: '$SOCKET_HOME/packages/@socketsupply/socket'"

  _publish

  if (( do_global_link )); then
    for arch in "${archs[@]}"; do
      declare package="@socketsupply/socket-$platform-${arch/x86_64/x64}"
      npm link --no-fund --no-audit --offline "$package"
    done

    npm link --no-fund --no-audit --offline
  fi
fi
