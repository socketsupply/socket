declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

# TODO(@mribbons): Test on darwin
# TODO(@mribbons): Test lib paths when building from create-socket-app

declare args=()
declare pids=()
declare force=0

declare CWD=$(pwd)
declare BUILD_DIR="$root/build"
declare LIB_DIR="$root/build"

declare arch="$(uname -m)"
declare host="$(uname -s)"
declare platform="desktop"

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
    pass_force="$arg"
    force=1; continue   
  fi

  args+=("$arg")
done

if [ -d "$SOCKET_HOME" ]; then
  LIB_DIR=$SOCKET_HOME/lib
fi

if [ -n "$LOCALAPPDATA" ] && [ -z "$SOCKET_HOME" ]; then
  SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
else
  SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"
fi

for sig in ABRT HUP INT PIPE QUIT TERM; do
  trap "echo command $@ failed $sig;
    trap - $sig EXIT && kill -s $sig $$
  " $sig
done

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

if [[ "$host" == "Win32" ]] && [[ ! -z "$SOCKET_HOME" ]] ; then
  # ndk build doesn't like wrong slashes, even tough it generates them...
  SOCKET_HOME_X=$SOCKET_HOME
  # Android requires SOCKET_HOME that contains all source files, that won't be the case by the time this script runs 
  SOCKET_HOME=$(cygpath -w $(dirname $(dirname $(which ssc))))

  if [[ ! -d $SOCKET_HOME_X ]]; then
    echo "SOCKET_HOME not found at $SOCKET_HOME_X"
    DEPS_ERROR=1
  fi
fi


if ! quiet command -v ssc; then
  echo "ssc not found."
  DEPS_ERROR=1
fi;

if [[ -z $ANDROID_HOME ]]; then
  echo "ANDROID_HOME not set."
  DEPS_ERROR=1
fi

if [[ -z $JAVA_HOME ]]; then
  echo "JAVA_HOME not set."
  DEPS_ERROR=1
fi

if [[ ! -z $DEPS_ERROR ]]; then
  echo "Dependencies not satisfied."
  exit 1
fi


app_dir=$BUILD_DIR/android
if [[ -d $app_dir ]] && (( $force )); then
  quiet rm -rf $app_dir
fi

if [[ ! -d $app_dir ]]; then
  quiet mkdir -p $app_dir
  cd $app_dir
  echo "Building Android App"
  quiet ssc init
  temp=$(mktemp)
  quiet sed '/\[android\]/s/.*/&\
  build_remove_path = dist\/android\/app\/src\//' $app_dir/socket.ini > $temp
  quiet mv $temp $app_dir/socket.ini
  quiet sed '/\[android\]/s/.*/&\
  skip_gradle = true/' $app_dir/socket.ini > $temp
  quiet mv $temp $app_dir/socket.ini
  echo "starting ssc from $(pwd)"
  if ! quiet ssc build -o --platform=android > build.log 2>&1; then
    cat build.log
    echo "NDK build failed."
    exit 1
  fi
else
  echo "# Not rebuilding $app_dir"
fi

for abi in $(ls $app_dir/dist/android/app/src/main/obj/local)
do
  [ ! -z $VERBOSE ] && echo "copying $abi to $LIB_DIR/$abi-android"
  quiet mkdir -p $LIB_DIR/$abi-android/lib
  quiet cp -rf $app_dir/dist/android/app/src/main/jniLibs/$abi/* $LIB_DIR/$abi-android/lib
done
