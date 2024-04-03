#!/bin/sh

version="${1:-"1.0.23-0"}"

rm -rf api/stream-relay.js || exit $?
rm -rf api/stream-relay || exit $?
cp -rf node_modules/@socketsupply/stream-relay/src api/stream-relay || exit $?
rm -rf node_modules/@socketsupply/{socket,socket-{darwin,linux,win32}*,stream-relay} || exit $?

for file in $(find api/stream-relay -type f); do
  sed -i.bak -e "s/'socket:\(.*\)'/'..\/\1.js'/g" "$file" && rm "$file.bak" || exit $?
done

{
  echo "import def from './stream-relay/index.js'"
  echo "export * from './stream-relay/index.js'"
  echo "export default def"
} >> api/stream-relay.js

tree api/stream-relay
