$OLD_CWD = (Get-Location).Path

$TEMP_PATH = Join-Path $Env:Temp $(New-Guid)
(New-Item -Type Directory -Path $TEMP_PATH) > $null

$INSTALL_PATH = "$env:LOCALAPPDATA\Programs\optoolco"
$WORKING_PATH = $OLD_CWD

if (Test-Path -Path $INSTALL_PATH) {
    Remove-Item -Recurse -Force $INSTALL_PATH
    Write-Output "$([char]0x2666) Cleaned $INSTALL_PATH"
}

try {
    (New-Item -ItemType Directory -Force -Path $INSTALL_PATH) > $null
} catch {
    Write-Error "Unable to create directory '$INSTALL_PATH'"
    Exit 1
}

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
    Copy-Item -Path "$WORKING_PATH\src\*" -Destination $INSTALL_PATH\src -Recurse
}

#
# Set up the path for the user, cool Windows feature actually.
#
Function Install-Path {
    $IN_PATH=$env:Path -split ';' -contains $INSTALL_PATH

    if ($IN_PATH -ne 1) {
        If (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {    
            Write-Output "$([char]0x2666) You are not admin, so the path can not be modified. It's safe to run this script again as admin."
            break
        }

        Write-Output "$([char]0x2666) Installing Path."
        $OLD_PATH = (Get-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH).path
        Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH -Value $INSTALL_PATH
    }
}

if ($args.Count -eq 0) {
    Build
    Install-Files
    Install-Path
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
Install-Path
Install-Files

cd $OLD_CWD