#!/bin/sh

version="${1:-"1.0.23-0"}"
npm i @socketsupply/stream-relay@"$version" --registry https://npm.pkg.github.com || exit $?
rm -rf api/stream-relay || exit $?
cp -rf node_modules/@socketsupply/stream-relay/src api/stream-relay || exit $?

for file in $(find api/stream-relay -type f); do
  sed -i '' -e "s/'socket:\(.*\)'/'..\/\1.js'/g" "$file" || exit $?
done

tree api/stream-relay
