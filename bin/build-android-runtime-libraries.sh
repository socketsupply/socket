declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

### How this script is used
# 1. install.sh is executed
# 2. If ssc is installed && ANDROID_HOME is set, _build_runtime_library() calls build-runtime-library.sh --platform android & pids+=($!)
# This will happen if we're running from sh bin\publish-npm-modules.sh: This provides some performance benefit as 
# 3. If platform == android, build-runtime-library.sh calls this script and returns the result without any further processing
# android libs can be built in parallel with others
# 4. This script produces libs in the same folder lib/$arch-$platform format as build-runtime-library.sh
# 5. At the end of install.sh, build-runtime-library.sh --platform android is attempted again
# (Note that there are checks in place to ensure that the build doesn't happen twice)

### How this script works
# Currently it does not call ndkbuild directly.
# Instead, it leverages off the previously existing cli.cc android code to:
# a. Download android dependencies
# b. BUild libsocket-runtime.so for each android abi

# To achieve this, new [android] options were added to socket.ini
# build_socket_runtime - (default false) - if enabled call ndkbuild to create libsocket-runtime.so
#                                        - if disabled, copy android libs from $SOCKET_HOME
# skip_gradle - (default false) - If enabled, don't compile app or build apk

# Therefore, in order to produce libsocket-runtime.so's, this script just has to enable 
# the options above in socket.ini.

# When the user wants to build an app, build_socket_runtime is disabled so 
# prebuilt libraries will be copied to their app before the apk is bundled.

# Requirements
# This applies to both building libs and apps
# Standard Android development environment (IDEs not required):
# $ANDROID_HOME which points to a folder containing cmdline-tools/latest and platform-tools
# $JAVA_HOME which points to a folder containing bin\javac ~v19 (NOT the bin folder itself)
# gradle > 8.0.1 in $PATH
# ndk tools will be downloaded during build process

### Why do this?
# 1. Using gradle, there is no way to pass -J (parallel build) to ndkbuild, so it's slow
# 2. We should prebuild the runtime for Android as we do on other platforms

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


if ! quiet command -v ssc; then
  echo "ssc not found."
  DEPS_ERROR=1
fi;

if [[ "$host" == "Win32" ]] && [[ ! -z "$SOCKET_HOME" ]] && [[ -z "$DEPS_ERROR" ]] ; then
  # ndk build doesn't like wrong slashes, even tough it generates them...
  SOCKET_HOME_X=$SOCKET_HOME
  # Android requires SOCKET_HOME that contains all source files, that won't be the case by the time this script runs 
  # Don't run this command if ssc not found, it throws a lot of errors that are hard to redirect
  SOCKET_HOME=$(cygpath -w $(dirname $(dirname $(which ssc))))

  if [[ ! -d $SOCKET_HOME_X ]]; then
    echo "SOCKET_HOME not found at $SOCKET_HOME_X"
    DEPS_ERROR=1
  fi
fi

if [[ -z $ANDROID_HOME ]]; then
  echo "ANDROID_HOME not set."
  DEPS_ERROR=1
fi

if [[ -z $JAVA_HOME ]]; then
  echo "JAVA_HOME not set."
  DEPS_ERROR=1
fi

# TODO(@mribbons):Test gradle version > 8.0.1
if ! quiet command -v gradle; then
  # gradle required for apps, but not for libsocket-runtime.so
  echo "gradle not in path."
  DEPS_ERROR=1
fi

if [[ ! -z $DEPS_ERROR ]]; then
  echo "Android dependencies not satisfied."
  exit 1
fi

app_dir=$BUILD_DIR/android
obj_dir=$app_dir/dist/android/app/src/main/obj/local
if [[ -d $app_dir ]] && (( $force )); then
  quiet rm -rf $app_dir
fi

if [[ ! -d $obj_dir ]]; then
  quiet mkdir -p $app_dir
  cd $app_dir
  echo "Building Android App"
  quiet ssc init
  temp=$(mktemp)
  # Using quiet with > is bad news
  quiet echo "Applying NDK build options..."
  sed '/\[android\]/s/.*/&\
build_remove_path = dist\/android\/app\/src\/\
build_socket_runtime = true\
skip_gradle = true/' $app_dir/socket.ini > $temp
  mv $temp $app_dir/socket.ini
  option_test="$(grep "build_socket_runtime = true" $app_dir/socket.ini|wc -l)"
  if [[ "$option_test" -lt "1" ]]; then
    echo "$app_dir/socket.ini doesn't contain 'build_socket_runtime = true', ssc or sed command failed."
    exit 1
  fi
  echo "Starting ssc from $(pwd), this might take some time."
  # downloads android deps
  quiet echo "ssc build -o --platform=android > build.log 2>&1"
  if ! ssc build -o --platform=android > build.log 2>&1; then
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
