#!/bin/bash

LATEST_HASH=`git log --pretty=format:'%h' -n 1`

BASE_STRING=`cat VERSION.txt`
BASE_LIST=(`echo $BASE_STRING | tr '.' ' '`)
V_MAJOR=${BASE_LIST[0]}
V_MINOR=${BASE_LIST[1]}
V_PATCH=${BASE_LIST[2]}
echo -e "Current version: $BASE_STRING"
echo -e "Latest commit hash: $LATEST_HASH"
V_MINOR=$((V_MINOR + 1))
V_PATCH=0
SUGGESTED_VERSION="$V_MAJOR.$V_MINOR.$V_PATCH"
echo -ne "Enter a version number [$SUGGESTED_VERSION]: "
read INPUT_STRING
if [ "$INPUT_STRING" = "" ]; then
    INPUT_STRING=$SUGGESTED_VERSION
fi
echo -e "Will set new version to be $INPUT_STRING"
echo $INPUT_STRING > VERSION.txt
jq ".version = \"$INPUT_STRING\"" clib.json > tmp.$$.json && mv tmp.$$.json clib.json
git add VERSION.txt
git commit -m "Bump version to ${INPUT_STRING}."
# git tag -a -m "Tag version ${INPUT_STRING}." "v$INPUT_STRING"
# git push origin --tags

echo -e "Finished."
