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
  Exit 0
} else {
  Write-Output "Version check has failed"
  Write-Output "Expected: $VERSION_EXPECTED"
  Write-Output "Got: $VERSION_SSC"
  Exit 1
}
