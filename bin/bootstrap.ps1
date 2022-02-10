if ($args[0] -eq "test-sign") {
  MakeCert.exe -sv cert.pvk -n "CN=Operator Tool Co, O=Operator, L=New York, S=New York, C=US" cert.cer -r -a sha256
  pvk2pfx.exe -pvk cert.pvk -pi test -spc cert.cer -pfx cert.pfx
  $env:CSC_KEY_PASSWORD = 'test'
  .\bin\opkit.exe .\example\desktop -p -c
  Remove-Item cert.cer
  Remove-Item cert.pfx
  Remove-Item cert.pvk
  Exit 0
}

$OLD_CWD = (Get-Location).Path

$TEMP_PATH = Join-Path $Env:Temp $(New-Guid)
(New-Item -Type Directory -Path $TEMP_PATH) > $null

$INSTALL_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\src"
$WORKING_PATH = $OLD_CWD

Write-Output ""
Write-Output "Consider adding '$env:LOCALAPPDATA\Programs\socketsupply' to your path."
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
    clang++ src\cli.cc -o $WORKING_PATH\bin\opkit.exe -std=c++20 -DVERSION="$($VERSION)"
    # -I 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\shared' `
    # -I 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\um' `
    # -I 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\winrt' `
    # -I 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\cppwinrt'

    if ($? -ne 1) {
        Write-Output "$([char]0x2666) The build tool failed to compile. Here's what you can do..."
        Exit 1
    }
}

#
# Install the files we will want to use for builds
#
Function Install-Files {
  Write-Output "$([char]0x2666) Installing Files to '$INSTALL_PATH'."

  Copy-Item $WORKING_PATH\bin\opkit.exe -Destination $INSTALL_PATH
  Copy-Item -Path "$WORKING_PATH\src\*" -Destination $INSTALL_PATH -Recurse -Container
}

Write-Output "$([char]0x2666) Working path set to $WORKING_PATH."

if ($args[0] -eq "update") {
    Write-Output "$([char]0x2666) Updating WebView2 header files..."
    #https://docs.microsoft.com/en-us/microsoft-edge/webview2/concepts/versioning
    #PACKAGE_VERSION='1.0.992.28'
    $PACKAGE_VERSION='1.0.1133-prerelease'
    Invoke-WebRequest https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$PACKAGE_VERSION -O $TEMP_PATH\webview2.zip
    Expand-Archive -Path $TEMP_PATH\WebView2.zip -DestinationPath $TEMP_PATH\WebView2
    Copy-Item -Path $TEMP_PATH\WebView2\build\native\include\WebView2.h $WORKING_PATH\src\win64
    Copy-Item -Path $TEMP_PATH\WebView2\build\native\include\WebView2Experimental.h $WORKING_PATH\src\win64
    Copy-Item -Path $TEMP_PATH\WebView2\build\native\x64\WebView2LoaderStatic.lib $WORKING_PATH\src\win64
    Exit 0
}

if ($args.Count -eq 0) {
  $WORKING_PATH = $TEMP_PATH
}

if ($args.Count -ne 0) {
  Build
  Install-Files
  Exit 0
}

Write-Output "$([char]0x2666) Checking for compiler."
(Get-Command clang++) > $null

if ($? -ne 1) {
    Write-Output "$([char]0x2666) Installing compiler..."
    Invoke-WebRequest https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/LLVM-13.0.0-win64.exe -O llvm.exe
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
Write-Output "$([char]0x2666) Fetching files to '$WORKING_PATH'..."
Remove-Item -Recurse -Force $WORKING_PATH
(git clone --depth=1 git@github.com:socketsupply/opkit.git "$($WORKING_PATH)") > $null

cd $WORKING_PATH

Build
Install-Files

cd $OLD_CWD
