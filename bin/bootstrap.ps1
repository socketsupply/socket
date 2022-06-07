#
# TODO convert this to a nuget package
# https://docs.microsoft.com/en-us/nuget/create-packages/creating-a-package
#
$OLD_CWD = (Get-Location).Path

$TEMP_PATH = Join-Path $Env:Temp $(New-Guid)
(New-Item -Type Directory -Path $TEMP_PATH) > $null

$INSTALL_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\src"
$LIB_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\lib"
$WORKING_PATH = $OLD_CWD

if (Test-Path -Path $INSTALL_PATH) {
  Remove-Item -Recurse -Force $INSTALL_PATH
  Write-Output "- Cleaned $INSTALL_PATH"
}

if (Test-Path -Path $LIB_PATH) {
  Remove-Item -Recurse -Force $LIB_PATH
  Write-Output "- Cleaned $LIB_PATH"
}

(New-Item -ItemType Directory -Force -Path $INSTALL_PATH) > $null
Write-Output "- Created $INSTALL_PATH"

(New-Item -ItemType Directory -Force -Path $LIB_PATH) > $null
Write-Output "- Created $LIB_PATH"

#
# Compile with the current git revision of the repository
#
Function Build {
  $VERSION_HASH = $(git rev-parse --short HEAD) 2>&1 | % ToString
  $VERSION = $(type VERSION.txt) 2>&1 | % ToString

  Write-Output "- Compiling the build tool"
  clang++ src\cli.cc -o $WORKING_PATH\bin\ssc.exe -std=c++20 -DVERSION_HASH="$($VERSION_HASH)" -DVERSION="$($VERSION)"
  # -I 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\shared' `

  if ($? -ne 1) {
    Write-Output "- The build tool failed to compile. Here's what you can do..."
    Exit 1
  }

  if ($env:Path -notlike "*$INSTALL_PATH*") {
    $NEW_PATH = "$INSTALL_PATH;$env:Path"
    $env:Path = $NEW_PATH

    # This only works if ran as administrator
    Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name path -Value $NEW_PATH -ErrorAction SilentlyContinue

    if ($?) {
      # This command creates duplicates for me
      # $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + [System.Environment]::GetEnvironmentVariable("Path","User")
      Write-Output "- ssc has been added to the path. Close you terminal to apply the changes."
    } else {
      Write-Output "- Command ssc has been added to the path for the current session."
      Write-Output ""
      Write-Output "Consider adding ssc to your path for other sessions temporarily:"
      Write-Output " `$env:Path += "";$INSTALL_PATH"""
      Write-Output "or add it to the registry to make it available globally (needs administrator rights):"
      Write-Output " Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name path -Value ""$INSTALL_PATH;`$env:Path"""
      Write-Output ""
    }
  }
}

#
# Install the files we will want to use for builds
#
Function Install-Files {
  Write-Output "- Installing Files to '$INSTALL_PATH'."
  Copy-Item $WORKING_PATH\bin\ssc.exe -Destination $INSTALL_PATH
  Copy-Item -Path "$WORKING_PATH\src\*" -Destination $INSTALL_PATH -Recurse -Container

  Write-Output "- Installing Files to '$LIB_PATH'."
  Copy-Item -Path "$WORKING_PATH\lib\*" -Destination $LIB_PATH -Recurse -Container
}

Write-Output "- Working path set to $WORKING_PATH."

if ($args[0] -eq "update") {
  $PACKAGE_VERSION = '1.0.1133-prerelease'
  $base = "$TEMP_PATH\WebView2\build\native"

  Write-Output "- Updating WebView2 header files..."
  #https://docs.microsoft.com/en-us/microsoft-edge/webview2/concepts/versioning
  Invoke-WebRequest https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$PACKAGE_VERSION -O $TEMP_PATH\webview2.zip
  Expand-Archive -Path $TEMP_PATH\WebView2.zip -DestinationPath $TEMP_PATH\WebView2
  Copy-Item -Path $base\include\WebView2.h $WORKING_PATH\src\win64
  Copy-Item -Path $base\include\WebView2EnvironmentOptions.h $WORKING_PATH\src\win64
  Copy-Item -Path $base\include\WebView2Experimental.h $WORKING_PATH\src\win64
  Copy-Item -Path $base\x64\WebView2LoaderStatic.lib $WORKING_PATH\src\win64

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

Write-Output "- Checking for compiler."
(Get-Command clang++) > $null

if ($? -ne 1) {
  Write-Output "- Installing compiler..."
  Invoke-WebRequest https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/LLVM-13.0.0-win64.exe -O llvm.exe
  llvm.exe
}

Write-Output "- Checking for git."
(Get-Command git) > $null

if ($? -ne 1) {
  Write-Output "- Please install git."
  Exit 1
}

#
# Get clone to a temp dir and navigate into it
#
Write-Output "- Fetching files to '$WORKING_PATH'..."
Remove-Item -Recurse -Force $WORKING_PATH
(git clone --depth=1 git@github.com:socketsupply/socket-sdk.git "$($WORKING_PATH)") > $null

cd $WORKING_PATH

Build
Install-Files

cd $OLD_CWD
