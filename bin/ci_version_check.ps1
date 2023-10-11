$BIN_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\bin"

if ($env:Path -notlike "*$BIN_PATH*") {
  $new_path = "$BIN_PATH;$env:Path"
  $env:Path = $new_path
}

$VERSION_SSC = $(ssc -v 2>&1 | Select-Object -First 1) | % ToString
$VERSION_TXT = $(type VERSION.txt) 2>&1 | % ToString
$VERSION_GIT = $(git rev-parse --short=8 HEAD) 2>&1 | % ToString
$VERSION_EXPECTED = "$VERSION_TXT ($VERSION_GIT)"

if ($VERSION_SSC -eq $VERSION_EXPECTED) {
  Write-Output "Version check has passed"
} else {
  Write-Output "Version check has failed"
  Write-Output "Expected: $VERSION_EXPECTED"
  Write-Output "Got: $VERSION_SSC"
  Exit 1
}

$BASE_LIST = $VERSION_TXT -split '\.'

$V_MAJOR = $BASE_LIST[0]
$V_MINOR = $BASE_LIST[1]

$VERSION_NODE = npm show ./npm/packages/@socketsupply/socket-node version

$BASE_LIST = $VERSION_NODE -split '\.'

$V_MAJOR_NODE = $BASE_LIST_NODE[0]
$V_MINOR_NODE = $BASE_LIST_NODE[1]

if (($V_MAJOR -ne $V_MAJOR_NODE) -or ($V_MINOR -ne $V_MINOR_NODE)) {
    Write-Output "Version of @socketsupply/socket-node is not in sync with @socketsupply/socket"
    Write-Output "@socketsupply/socket version is $VERSION_TXT"
    Write-Output "@socketsupply/socket-node version is $VERSION_NODE"
    exit 1
}
