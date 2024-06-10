#!/bin/sh

npm link @socketsupply/latica

version="${1:-"1.0.23-0"}"

rm -rf api/latica.js || exit $?
rm -rf api/latica || exit $?
cp -rf node_modules/@socketsupply/latica/src api/latica || exit $?
rm -rf node_modules/@socketsupply/{socket,socket-{darwin,linux,win32}*,latica} || exit $?

for file in $(find api/latica -type f); do
  sed -i '' -e "s/'socket:\(.*\)'/'..\/\1.js'/g" "$file" || exit $?
done

{
  echo "import def from './latica/index.js'"
  echo "export * from './latica/index.js'"
  echo "export default def"
} >> api/latica.js

tree api/latica
