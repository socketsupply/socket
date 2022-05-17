$VERSION_OP = $(op -v) 2>&1 | % ToString
$VERSION_TXT = $(type VERSION.txt) 2>&1 | % ToString
$VERSION_GIT = $(git rev-parse --short HEAD) 2>&1 | % ToString
$VERSION_EXPECTED = "$VERSION_TXT ($VERSION_GIT)"

if ($VERSION_OP -eq $VERSION_EXPECTED) {
  Write-Output "Version check has passed"
  Exit 0
} else {
  Write-Output "Version check has failed"
  Write-Output "Expected: $VERSION_EXPECTED"
  Write-Output "Got: $VERSION_OP"
  Exit 1
}
