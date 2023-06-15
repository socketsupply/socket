#!/usr/bin/env bash

root="$(CDPATH='' cd -- "$(dirname "$(dirname -- "$0")")" && pwd)"

build_dir="$root/build"
sqlite3_build_dir="$root/build/sqlite3"
sqlite3_zip_file="$sqlite3_build_dir/sqlite3.zip"
sqlite3_download_url="https://sqlite.org/2023/sqlite-amalgamation-3420000.zip"

if ! test -f "$sqlite3_zip_file"; then
  mkdir -p "$sqlite3_build_dir"
  curl -sL "$sqlite3_download_url" -o "$sqlite3_zip_file"
fi

if test -f "$sqlite3_zip_file"; then
  pushd . >/dev/null || exit $?
  cd "$sqlite3_build_dir" || exit $?
  rm -f *.c *.h || exit $?
  unzip -q "$sqlite3_zip_file" || exit $?
  mv sqlite-*/*.{c,h} . || exit $?
  rm -rf sqlite-* || exit $?
  popd >/dev/null || exit $?
fi
