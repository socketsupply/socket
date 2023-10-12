#!/usr/bin/env bash

##
# Mustache templates for bash (https://github.com/jwerle/mush)
#
# The MIT License (MIT)
#
# Copyright (c) 2013 Joseph Werle
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
##

mush_version () {
  echo "0.1.4"
}

mush_usage () {
  echo "usage: mush [-ehV] [-f <file>] [-o <file>]"

  if [ "$1" = "1" ]; then
    echo
    echo "examples:"
    echo "  $ cat file.ms | FOO=BAR mush"
    echo "  $ VALUE=123 mush -f file.ms -o file"
    echo "  $ echo \"Today's date is {{DATE}}\" | DATE=\`date +%D\` mush"
    echo "  $ cat ./template.ms | VAR=VALUE mush"
    echo
    echo "options:"
    echo "  -f, --file <file>       file to parse"
    echo "  -o, --out <file>        output file"
    echo "  -e, --escape            escapes html html entities"
    echo "  -h, --help              display this message"
    echo "  -V, --version           output version"
  fi
}

mush () {
  # shellcheck disable=SC2034
  local SELF="$0"
  # shellcheck disable=SC2034
  local NULL=/dev/null
  # shellcheck disable=SC2034
  local STDIN=0
  local STDOUT=1
  local STDERR=2
  local LEFT_DELIM="{{"
  local RIGHT_DELIM="}}"
  # shellcheck disable=SC2034
  local INDENT_LEVEL="  "
  local ESCAPE=0
  local ENV=""
  local out=">&$STDOUT"

  ENV="$(env)"

  ## parse opts
  while true; do
    arg="$1"

    if [ "" = "$1" ]; then
      break;
    fi

    if [ "${arg:0:1}" != "-" ]; then
      shift
      continue
    fi

    case $arg in
      -f|--file)
        file="$2";
        shift 2;
        ;;
      -o|--out)
        out="> $2";
        shift 2;
        ;;
      -e|--escape)
        ESCAPE=1
        shift
        ;;
      -h|--help)
        mush_usage 1
        exit 1
        ;;
      -V|--version)
        mush_version
        exit 0
        ;;
      *)
        {
          echo "unknown option \`$arg'"
        } >&$STDERR
        mush_usage
        exit 1
        ;;
    esac
  done

  ## read each line
  while IFS= read -r line; do
    printf '%q\n' "${line}" | {
        ## read each ENV variable
        echo "$ENV" | {
          while read -r var; do
            ## split each ENV variable by '='
            ## and parse the line replacing
            ## occurrence of the key with
            ## guarded by the values of
            ## `LEFT_DELIM' and `RIGHT_DELIM'
            ## with the value of the variable
            case "$var" in
              (*"="*)
                key=${var%%"="*}
                val=${var#*"="*}
                ;;

              (*)
                key=$var
                val=
                ;;
            esac

            line="${line//${LEFT_DELIM}$key${RIGHT_DELIM}/$val}"
          done

          if [ "1" = "$ESCAPE" ]; then
            line="${line//&/&amp;}"
            line="${line//\"/&quot;}"
            line="${line//\</&lt;}"
            line="${line//\>/&gt;}"
          fi

          ## output to stdout
          echo "$line" | {
            ## parse undefined variables
            sed -e "s#${LEFT_DELIM}[A-Za-z]*${RIGHT_DELIM}##g" | \
            ## parse comments
            sed -e "s#${LEFT_DELIM}\!.*${RIGHT_DELIM}##g"
          };
        }
    };
  done
}


if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
  export -f mush
else
  if [ ! -t 0 ]; then
    eval "mush $out"
  elif [ -n "$file" ]; then
    eval "cat $file | mush $out"
  elif (( $# > 0 )); then
    mush "$@"
  else
    mush_usage
    exit 1
  fi
  exit $?
fi
