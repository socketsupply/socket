#!/usr/bin/env bash

root="$(CDPATH='' cd -- "$(dirname "$(dirname -- "$0")")" && pwd)"

build_dir="$root/build"
sqlite3_build_dir="$root/build/sqlite3"
sqlite3_zip_file="$sqlite3_build_dir/sqlite3.zip"
sqlite3_download_url="https://sqlite.org/2023/sqlite-amalgamation-3420000.zip"

if ! test -f "$sqlite3_zip_file"; then
  mkdir -p "$sqlite3_build_dir"
  curl "$sqlite3_download_url" -o "$sqlite3_zip_file"
fi

if test -f "$sqlite3_zip_file"; then
  pushd . >/dev/null
  cd "$sqlite3_build_dir"
  pwd
  rm -f *.c *.h
  unzip "$sqlite3_zip_file"
  mv sqlite-*/*.{c,h} .
  rm -rf sqlite-*
  popd >/dev/null
fi
