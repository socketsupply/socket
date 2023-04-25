#!/usr/bin/env bash

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

cd "$root" || exit $?

tsc --emitDeclarationOnly --module es2022 --outFile api/index || exit $?

sed -i '' -e 's/declare module "\(.*\)"/declare module "socket:\1"/g' api/index.d.ts || exit $?
sed -i '' -e 's/namespace \_\_\_.*$//g' api/index.d.ts || exit $?
