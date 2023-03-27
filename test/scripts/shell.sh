root="$(CDPATH='' cd -- "$(dirname "$(dirname -- "$0")")" && pwd)"
filename="$1"; shift

if [ "$(uname)" = "Linux" ] || [ "$(uname)" = "Darwin" ]; then
  env bash "$root/$filename" $@
else
  sh "$root/$filename" $@
fi

exit $?
