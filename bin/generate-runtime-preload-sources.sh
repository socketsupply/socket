#!/usr/bin/env bash

declare dirname="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
declare srcdir="$(cd "$dirname/../src" && pwd)"
declare output="$srcdir/core/runtime-preload-sources.hh"

declare PRELOAD_GENERATION_DATE=""

PRELOAD_GENERATION_DATE="$(date)"

function emit () {
  local name="$1"
  local file="$2"

  echo "// file= $(basename "$file")"
  echo "constexpr auto ${name} =" 'R"JS('
  cat "$file"
  echo ')JS";'
  echo
}

function generate-runtime-preload-sources () {
  rm -f "$output"

  {
    cat << PRE
#ifndef RUNTIME_PRELOAD_SOURCES_HH
#define RUNTIME_PRELOAD_SOURCES_HH

namespace SSC {
//
// THIS FILE WAS AUTO GENERATED ON: $PRELOAD_GENERATION_DATE
//
// This file contains JavaScipt that is injected into the webview before
// any user code is executed.
//

PRE

    emit "gPreload" "$srcdir/api/runtime.js"
    emit "gPreloadDesktop" "$srcdir/api/desktop.js"
    emit "gPreloadMobile" "$srcdir/api/mobile.js"

    cat << PRE
} // namespace SSC
#endif
PRE

  } > "$output"
}

if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
  export -f generate-runtime-preload-sources
else
  generate-runtime-preload-sources
  exit $?
fi
