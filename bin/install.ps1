$OLD_CWD = (Get-Location).Path

$SRC_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\src"
$WORKING_PATH = $OLD_CWD

#
# Compile with the current git revision of the repository
#
Function Build {
  $VERSION_HASH = $(git rev-parse --short HEAD) 2>&1 | % ToString
  $VERSION = $(type VERSION.txt) 2>&1 | % ToString
  $BUILD_TIME = [int] (New-TimeSpan -Start (Get-Date "01/01/1970") -End (Get-Date)).TotalSeconds
  $BUILD_PATH = "$WORKING_PATH\build"

  if (-not (Test-Path -Path $BUILD_PATH -PathType Container)) {
    (git clone -q --depth=1 git@github.com:libuv/libuv.git $BUILD_PATH) > $null
    (New-Item -ItemType Directory -Force -Path "$BUILD_PATH\lib") > $null
    Write-Output "ok - cloned libuv $BUILD_PATH"

    cd "$BUILD_PATH\lib"
    Write-Output "# bulding libuv..."
    (cmake ..) > $null
    cd "$WORKING_PATH"
    (cmake --build "$BUILD_PATH\lib" --config Release) > $null
    Write-Output "ok - built libuv"

    Copy-Item -Path "$BUILD_PATH\include\*" -Destination $SRC_PATH -Recurse -Container
    Copy-Item $BUILD_PATH\lib\Release\uv_a.lib -Destination $SRC_PATH\uv
  }

  Write-Output "# compiling the build tool..."
  clang++ src\cli.cc -o $WORKING_PATH\bin\ssc.exe -std=c++2a -DBUILD_TIME="$($BUILD_TIME)" -DVERSION_HASH="$($VERSION_HASH)" -DVERSION="$($VERSION)"
  # -I 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\shared' `

  if ($? -ne 1) {
    Write-Output "not ok - the build tool failed to compile. Here's what you can do..."
    Exit 1
  }

  if ($env:Path -notlike "*$SRC_PATH*") {
    $NEW_PATH = "$SRC_PATH;$env:Path"
    $env:Path = $NEW_PATH
    Write-Output "ok - command ssc has been added to the path for the current session."
    Write-Output ""
    Write-Output "# consider adding ssc to your path for other sessions:"
    Write-Output " `$env:Path = ""$SRC_PATH;`$env:Path"""
    Write-Output ""

    # Dangerous!
    # $REGISTRY = "Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment"
    # $OLD_PATH = (Get-ItemProperty -Path "$REGISTRY" -Name PATH).Path
    # $NEW_PATH= $SRC_PATH + ";" + $OLD_PATH
    # # This only works if ran as administrator
    # Set-ItemProperty -Path "$REGISTRY" -Name path -Value $NEW_PATH -ErrorAction SilentlyContinue

    # if ($?) {
    #   # This command creates duplicates for me
    #   # $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    #   Write-Output "- ssc has been added to the path. Close you terminal to apply the changes."
    # } else {
    #   Write-Output "- Command ssc has been added to the path for the current session."
    #   Write-Output ""
    #   Write-Output "Consider adding ssc to your path for other sessions temporarily:"
    #   Write-Output " `$env:Path = ""$SRC_PATH;`$env:Path"""
    #   Write-Output "or add it to the registry to make it available globally (needs administrator rights):"
    #   Write-Output " Set-ItemProperty -Path ""$REGISTRY"" -Name path -Value ""$NEW_PATH"""
    #   Write-Output ""
    # }
  }
}

#
# Install the files we will want to use for builds
#
Function Install-Files {
  Copy-Item $WORKING_PATH\bin\ssc.exe -Destination $SRC_PATH
  Copy-Item -Path "$WORKING_PATH\src\*" -Destination $SRC_PATH -Recurse -Force
  Copy-Item -Path "$WORKING_PATH\src\win64\*" -Destination $SRC_PATH -Recurse -Force
  Write-Output "ok - installed files to '$SRC_PATH'."
}

#
# Get and extract just the WebView2 files that we need
#
Function Install-WebView2 {
  $TEMP_PATH = Join-Path $Env:Temp $(New-Guid)
  (New-Item -Type Directory -Path $TEMP_PATH) > $null

  $PACKAGE_VERSION = '1.0.1248-prerelease'
  $base = "$TEMP_PATH\WebView2\build\native"

  Write-Output "# updating WebView2 header files..."
  #https://docs.microsoft.com/en-us/microsoft-edge/webview2/concepts/versioning
  Invoke-WebRequest https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/$PACKAGE_VERSION -O $TEMP_PATH\webview2.zip
  Expand-Archive -Path $TEMP_PATH\WebView2.zip -DestinationPath $TEMP_PATH\WebView2
  Copy-Item -Path $base\include\WebView2.h $WORKING_PATH\src\win64
  Copy-Item -Path $base\include\WebView2EnvironmentOptions.h $WORKING_PATH\src\win64
  Copy-Item -Path $base\include\WebView2Experimental.h $WORKING_PATH\src\win64
  Copy-Item -Path $base\x64\WebView2LoaderStatic.lib $WORKING_PATH\src\win64
  Write-Output "ok - updated WebView2 header files..."
}

(Get-Command "choco.exe" -ErrorAction SilentlyContinue) > $null

if ($? -ne 1) {
  $InstallDir='C:\ProgramData\chocoportable'
  $env:ChocolateyInstall="$InstallDir"

  Set-ExecutionPolicy Bypass -Scope Process -Force;
  iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
}

(Get-Command "clang++.exe" -ErrorAction SilentlyContinue) > $null

if ($? -ne 1) {
  choco install llvm --confirm --force

  if ($? -ne 1) {
    Write-Output "not ok - unable to install llvm"
    Exit 1
  }
}

(Get-Command "cmake.exe" -ErrorAction SilentlyContinue) > $null

if ($? -ne 1) {
  choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' --confirm --force

  if ($? -ne 1) {
    Write-Output "not ok - unable to install cmake"
    Exit 1
  }
}

(Get-Command "git.exe" -ErrorAction SilentlyContinue) > $null

if ($? -ne 1) {
  choco install git

  if ($? -ne 1) {
    Write-Output "not ok - unable to install git"
    Exit 1
  }
}

refreshenv

if ($args[0] -eq "update") {
  Install-WebView2
  Exit 0
}

if (-not (Test-Path -Path $SRC_PATH)) {
  (New-Item -ItemType Directory -Path $SRC_PATH) > $null
  Write-Output "ok - created $SRC_PATH"
}

Write-Output "# working path set to $WORKING_PATH"
cd $WORKING_PATH

Build
Install-Files

cd $OLD_CWD
