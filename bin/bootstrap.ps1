$OLD_CWD = (Get-Location).Path

$TEMP_PATH = Join-Path $Env:Temp $(New-Guid)
(New-Item -Type Directory -Path $TEMP_PATH) > $null

$INSTALL_PATH = "$env:LOCALAPPDATA\Programs\optoolco"
$WORKING_PATH = $OLD_CWD

Write-Output ""
Write-Output "Consider adding '%LOCALAPPDATA%\Programs\optoolco' to your path."
Write-Output ""

if (Test-Path -Path $INSTALL_PATH) {
    Remove-Item -Recurse -Force $INSTALL_PATH
    Write-Output "$([char]0x2666) Cleaned $INSTALL_PATH"
}

(New-Item -ItemType Directory -Force -Path $INSTALL_PATH) > $null
Write-Output "$([char]0x2666) Created $INSTALL_PATH"

#
# Compile with the current git revision of the repository
#
Function Build {
    $VERSION = cmd /c 'git rev-parse --short HEAD' 2>&1 | % ToString

    Write-Output "$([char]0x2666) Compiling the build tool"
    clang++ src\cli.cc -o bin\cli.exe -std=c++20 -DVERSION="$($VERSION)"

    if ($? -ne 1) {
        Write-Output "$([char]0x2666) The build tool failed to compile. Here's what you can do..."
        Exit 1
    }
}

#
# Install the files we will want to use for builds
#
Function Install-Files {
    Write-Output "$([char]0x2666) Installing Files."

    Copy-Item $WORKING_PATH\bin\cli.exe -Destination $INSTALL_PATH\opkit.exe
    Copy-Item -Path $WORKING_PATH\src\* -Destination $INSTALL_PATH\src -Recurse
    Copy-Item -Path $WORKING_PATH\src\* -Destination $INSTALL_PATH\src -Recurse -Container
}

if ($args.Count -eq 0) {
    Build
    Install-Files
    Exit 0
} else {
    WORKING_PATH = $TMPD
}

Write-Output "$([char]0x2666) Checking for compiler."
(Get-Command clang++) > $null

if ($? -ne 1) {
    Write-Output "$([char]0x2666) Installing compiler..."
    Invoke-WebRequest https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/LLVM-12.0.0-win64.exe -O llvm.exe
    llvm.exe
}

Write-Output "$([char]0x2666) Checking for git."
(Get-Command git) > $null

if ($? -ne 1) {
    Write-Output "$([char]0x2666) Please install git."
    Exit 1
}

#
# Get clone to a temp dir and navigate into it
#
Write-Output "$([char]0x2666) Fetching files..."

(git clone --depth=1 git@github.com:optoolco/opkit.git "$($WORKING_PATH)") > $null

cd $WORKING_PATH

Build
Install-Files

cd $OLD_CWD