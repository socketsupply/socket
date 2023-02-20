declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

declare args=()
declare pids=()
declare force=0

declare CWD=$(pwd)
declare BUILD_DIR="$CWD/build"

declare arch="$(uname -m)"
declare host="$(uname -s)"
declare platform="desktop"

if [ -n "$LOCALAPPDATA" ] && [ -z "$SOCKET_HOME" ]; then
  SOCKET_HOME="$LOCALAPPDATA/Programs/socketsupply"
else
  SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"
fi

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

if [[ "$host" == "Win32" ]]; then
  cmd_suffix=".cmd"
  exe=".exe"
fi

declare DEPS_ERROR=0

if [[ ! -d $SOCKET_HOME ]]; then
  echo "SOCKET_HOME not found at $SOCKET_HOME"
  DEPS_ERROR=1
fi

if ! quiet command -v "ssc"; then
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

if [[ ! "$DEPS_ERROR"=="0" ]]; then
  echo "Dependencies not satisfied: $DEPS_ERROR"
  exit 1
fi

app_dir=$BUILD_DIR/android
quiet mkdir -p $app_dir
cd $app_dir
echo "Building Android App"
quiet ssc init
temp=$(mktemp)
quiet sed '/android/s/.*/&\
skip_gradle = true/' $app_dir/socket.ini > $temp
quiet mv $temp $app_dir/socket.ini
if ! quiet ssc build -o --platform=android; then
  echo "NDK build failed."
  exit 1
fi

for abi in $(ls $app_dir/dist/android/app/src/main/obj/local)
do
  echo "copying $abi to $BUILD_DIR/$abi-android"
  quiet mkdir -p $BUILD_DIR/$abi-android
  quiet cp -rf $app_dir/dist/android/app/src/main/obj/local/$abi $BUILD_DIR/$abi-android
done
