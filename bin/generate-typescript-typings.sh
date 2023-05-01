#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

cd "$root" || exit $?

npx tsc --emitDeclarationOnly --module es2022 --outFile api/index.tmp || exit $?

cat api/index.tmp.d.ts \
  | sed 's/declare module "\(.*\)"/declare module "socket:\1"/g' \
  | sed 's/namespace \_\_\_.*$//g' > api/index.d.ts \
  || exit $?

rm -f api/index.tmp.d.ts
