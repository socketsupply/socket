$OLD_CURRENT_DIRECTORY = [Environment]::CurrentDirectory

#
# Compile with the current git revision of the repository
#
Function Build {
    $version = cmd /c 'git rev-parse --short HEAD' 2>&1 | % ToString

    Write-Output "$([char]0x2666) Compiling the buld tool"
    clang++ src/cli.cc -o bin/cli.exe -std=c++20 -DVERSION="$($version)"

    if ($? -ne 1) {
        Write-Output "$([char]0x2666) The build tool failed to compile. Here's what you can do..."
        Exit 1
    }
}

if ($args.Count -eq 0) {
    Build
    Exit 0
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
$TMPD = Join-Path $Env:Temp $(New-Guid)
(New-Item -Type Directory -Path $TMPD) > $null

Write-Output "$([char]0x2666) Fetching files..."

(git clone --depth=1 git@github.com:optoolco/opkit.git "$($TMPD)") > $null

cd $TMPD

Build

#
# Set up the path for the user, cool Windows feature actually.
#
$NEW_PATH = 'C:\Program Files/optoolco'
$IN_PATH = Test-Path -Path "$($NEW_PATH)"

if ($IN_PATH -ne 1) {
    Write-Output "$([char]0x2665) Setting up path."
    $OLD_PATH = (Get-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH).path
    Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH -Value $NEW_PATH
}

#
# Install the files we will want to use for builds
#
Write-Output "$([char]0x2665) Installing."

Copy-Item "$($TMPD)\bin\cli.exe" "$($NEW_PATH)\op.exe"
Copy-Item "$($TMPD)\src" "$($NEW_PATH)\src" -Recursive

cd $OLD_CURRENT_DIRECTORY