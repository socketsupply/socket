#!/usr/bin/env bash

function ci_version_check() {
  VERSION_SSC=$(ssc -v)

  VERSION_TXT=$(cat VERSION.txt)
  VERSION_GIT=$(git rev-parse --short=8 HEAD)
  VERSION_EXPECTED="$VERSION_TXT ($VERSION_GIT)"

  if [ "$VERSION_SSC" = "$VERSION_EXPECTED" ]; then
    echo "Version check has passed";
    exit 0;
  else
    echo "Version check has failed";
    echo "Expected: $VERSION_EXPECTED";
    echo "Got: $VERSION_SSC";
    exit 1;
  fi
}

function generate_api_import_map() {
  declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"
  declare files=($(cd "$root/api" && find *.js **/*.js **/**/*.js 2>/dev/null))
  declare length=${#files[@]}
  declare output=()

  output+=('{')
  output+=('  "imports": {')

  for (( i = 0; i < length; ++i )); do
    declare file="${files[$i]}"
    declare name="${file/.js/}"
    declare entry=''
    declare is_last=$(( i == length - 1 ))

    entry="\"socket:$name\": \"./socket/$file\""

    if (( ! is_last )); then
      entry+=','
    fi

    output+=("     $entry")
  done

  output+=('  }')
  output+=('}')

  for line in "${output[@]}"; do
    echo "$line"
  done
}

function uninstall() {
    declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

  declare PREFIX="${PREFIX:-"/usr/local"}"
  declare SOCKET_HOME="${SOCKET_HOME:-"${XDG_DATA_HOME:-"$HOME/.local/share"}/socket"}"

  declare files=($(find "$SOCKET_HOME"/{bin,include,lib,objects,src,uv} -type f 2>/dev/null))
  declare bins=("$PREFIX/bin/ssc")

  declare uninstalled=0

  for file in "${files[@]}"; do
    if test -f "$file"; then
      echo "# removing $file"
      rm -f "$file"
      (( uninstalled++ ))
    fi
  done

  for bin in "${bins[@]}"; do
    if test -f "$bin"; then
      echo "# removing $bin"
      rm -f "$bin"
      (( uninstalled++ ))
    fi
  done

  if (( uninstalled > 0 )); then
    echo "ok - uninstalled $uninstalled files"
  else
    exit 1
  fi
}

function version() {
  LATEST_HASH=$(git log --pretty=format:'%h' -n 1)

  BASE_STRING=$(cat VERSION.txt)
  BASE_LIST=($(echo $BASE_STRING | tr '.' ' '))
  V_MAJOR=${BASE_LIST[0]}
  V_MINOR=${BASE_LIST[1]}
  V_PATCH=${BASE_LIST[2]}
  echo -e "Current version: $BASE_STRING"
  echo -e "Latest commit hash: $LATEST_HASH"
  V_MINOR=$((V_MINOR + 1))
  V_PATCH=0
  SUGGESTED_VERSION="$V_MAJOR.$V_MINOR.$V_PATCH"
  echo -ne "Enter a version number [$SUGGESTED_VERSION]: "
  read INPUT_STRING
  if [ "$INPUT_STRING" = "" ]; then
      INPUT_STRING=$SUGGESTED_VERSION
  fi
  echo -e "Will set new version to be $INPUT_STRING"
  echo $INPUT_STRING > VERSION.txt
  jq ".version = \"$INPUT_STRING\"" clib.json > tmp.$$.json && mv tmp.$$.json clib.json
  git add VERSION.txt clib.json
  git commit -m "Bump version to ${INPUT_STRING}."
  # git tag -a -m "Tag version ${INPUT_STRING}." "v$INPUT_STRING"
  # git push origin --tags

  echo -e "Finished."
}

case $1 in
  --ci-version-check)
    ci_version_check
    ;;
  --generate-api-import-map)
    generate_api_import_map
    ;;
  --uninstall)
    uninstall
    ;;
  --version)
    version
    ;;
esac