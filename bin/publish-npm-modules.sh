#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
declare archs=($(uname -m))
declare platform="$(uname -s | tr '[[:upper:]]' '[[:lower:]]')"

declare args=()
declare dry_run=0
declare only_platforms=0
declare only_top_level=0
declare remove_socket_home=1
declare do_global_link=0

function _publish () {
  if (( !dry_run )) ; then
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

if (( !only_top_level ))  && (( !no_rebuild )) ; then
  "$root/bin/install.sh" || exit $?
fi

mkdir -p "$SOCKET_HOME/packages/@socketsupply"

if (( !only_platforms || only_top_level )); then
  declare package="@socketsupply/socket"
  cp -rf "$root/npm/packages/@socketsupply/socket" "$SOCKET_HOME/packages/$package"
  mkdir -p "$SOCKET_HOME/packages/$package/bin"
  cp -rf "$root/npm/bin/ssc.js" "$SOCKET_HOME/packages/$package/bin/ssc.js"
  cp -f "$root/LICENSE.txt" "$SOCKET_HOME/packages/$package"
  cp -f "$root/README.md" "$SOCKET_HOME/packages/$package/README-RUNTIME.md"
  cp -rf "$SOCKET_HOME/api"/* "$SOCKET_HOME/packages/$package"
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
    # cp -f $SOCKET_HOME/lib/*-android/*.a "$SOCKET_HOME/packages/$package/lib"

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
      cp -rap $SOCKET_HOME/ps1 $SOCKET_HOME/packages/$package
    fi

    cd "$SOCKET_HOME/packages/$package" || exit $?
    echo "# in directory: '$SOCKET_HOME/packages/$package'"

    _publish

    if (( do_global_link )); then
      npm link
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
      npm link "$package"
    done

    npm link
  fi
fi
