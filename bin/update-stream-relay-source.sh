#!/bin/sh

version="${1:-"1.0.23-0"}"

if ! test -d node_modules/@socketsupply/stream-relay; then
  npm i @socketsupply/stream-relay@"$version" --registry https://npm.pkg.github.com || exit $?
fi

rm -rf api/stream-relay.js || exit $?
rm -rf api/stream-relay || exit $?
cp -rf node_modules/@socketsupply/stream-relay/src api/stream-relay || exit $?
rm -rf node_modules/@socketsupply/{socket,socket-{darwin,linux,win32}*,stream-relay} || exit $?

for file in $(find api/stream-relay -type f); do
  sed -i '' -e "s/'socket:\(.*\)'/'..\/\1.js'/g" "$file" || exit $?
done

{
  echo "import def from './stream-relay/index.js'"
  echo "export * from './stream-relay/index.js'"
  echo "export default def"
} >> api/stream-relay.js

tree api/stream-relay
