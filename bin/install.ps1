param([Switch]$debug, [Switch]$skipwebview, $webview = "1.0.1619-prerelease", $uv = "v1.44.2")

$OLD_CWD = (Get-Location).Path

$ASSET_PATH = "$env:LOCALAPPDATA\Programs\socketsupply"
$SRC_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\src"
$LIB_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\lib"
$BIN_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\bin"
$INCLUDE_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\include"
$WORKING_PATH = $OLD_CWD
$WORKING_BUILD_PATH = "$WORKING_PATH\build"
$LIBUV_BUILD_TYPE = "Release"
$LIBUV_TAG = $uv
# see https://docs.microsoft.com/en-us/microsoft-edge/webview2/concepts/versioning
$WEBVIEW2_VERSION = "$webview"
$SSC_BUILD_OPTIONS = "-O2"

if ($debug -eq $true) {
  $LIBUV_BUILD_TYPE = "Debug"
  $SSC_BUILD_OPTIONS = "-g", "-O0"
}

#
# Compile with the current git revision of the repository
#
Function Build {
  $VERSION_HASH = $(git rev-parse --short HEAD) 2>&1 | % ToString
  $VERSION = $(type VERSION.txt) 2>&1 | % ToString
  $BUILD_TIME = [int] (New-TimeSpan -Start (Get-Date "01/01/1970") -End (Get-Date)).TotalSeconds

  if (-not (Test-Path -Path "$WORKING_BUILD_PATH\libuv" -PathType Container)) {
    (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH") > $null
    Write-Output "# cloning libuv at $LIBUV_TAG from github..."
    (git clone --branch=$LIBUV_TAG -q --depth=1 https://github.com/libuv/libuv.git $WORKING_BUILD_PATH\libuv) > $null 2> $null
    Write-Output "ok - cloned libuv into $WORKING_BUILD_PATH\libuv"
  }

  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\libuv\build") > $null

  Write-Output "# building libuv..."
  cd "$WORKING_BUILD_PATH\libuv\build"
  (cmake .. -DBUILD_TESTING=OFF) > $null

  cd "$WORKING_BUILD_PATH\libuv"
  (cmake --build "$WORKING_BUILD_PATH\libuv\build" --config $LIBUV_BUILD_TYPE) > $null
  Write-Output "ok - built libuv"

  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\lib") > $null
  if ((Test-Path -Path "$WORKING_BUILD_PATH\libuv\build\$LIBUV_BUILD_TYPE\uv_a.lib" -PathType Leaf)) {
    Copy-Item "$WORKING_BUILD_PATH\libuv\build\$LIBUV_BUILD_TYPE\uv_a.lib" -Destination "$WORKING_BUILD_PATH\lib\uv_a.lib"
    if ($debug -eq $true) {
      Copy-Item "$WORKING_BUILD_PATH\libuv\build\$LIBUV_BUILD_TYPE\uv_a.pdb" -Destination "$WORKING_BUILD_PATH\lib\uv_a.pdb"
    }
  } elseif ((Test-Path -Path "$WORKING_BUILD_PATH\libuv\build\uv_a.lib" -PathType Leaf)) {
    # Support older versions of cmake that don't build to --config type path on multibuild systems
    Copy-Item "$WORKING_BUILD_PATH\libuv\build\uv_a.lib" -Destination "$WORKING_BUILD_PATH\lib\uv_a.lib"
    if ($debug -eq $true) {
      Copy-Item "$WORKING_BUILD_PATH\libuv\build\uv_a.pdb" -Destination "$WORKING_BUILD_PATH\lib\uv_a.pdb"
    }
  } else {
    Write-Output "Stop: uv_a.lib not found"
    Exit 1
  }

  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\include") > $null
  Copy-Item -Path "$WORKING_BUILD_PATH\libuv\include\*" -Destination "$WORKING_BUILD_PATH\include" -Recurse -Force -Container

  cd "$WORKING_PATH"
  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\bin") > $null
  Write-Output "# compiling the build tool..."
  clang++ $SSC_BUILD_OPTIONS -Xlinker /NODEFAULTLIB:libcmt -I"$WORKING_BUILD_PATH\include" -L"$WORKING_BUILD_PATH\lib" src\process\win.cc src\cli\cli.cc -o $WORKING_BUILD_PATH\bin\ssc.exe -std=c++2a -DSSC_BUILD_TIME="$($BUILD_TIME)" -DSSC_VERSION_HASH="$($VERSION_HASH)" -DSSC_VERSION="$($VERSION)"

  if ($? -ne 1) {
    Write-Output "not ok - the build tool failed to compile. Here's what you can do..."
    Exit 1
  }

  if ($env:Path -notlike "*$BIN_PATH*") {
    $NEW_PATH = "$BIN_PATH;$env:Path"
    $env:Path = $NEW_PATH
    Write-Output "ok - command ssc has been added to the path for the current session."
    Write-Output ""
    Write-Output "# consider adding ssc to your path for other sessions:"
    Write-Output " `$env:Path = ""$BIN_PATH;`$env:Path"""
    Write-Output ""
  }
}

#
# Install the files we will want to use for builds
#
Function Install-Files {
  (New-Item -ItemType Directory -Force -Path "$BIN_PATH") > $null
  (New-Item -ItemType Directory -Force -Path "$LIB_PATH") > $null
  (New-Item -ItemType Directory -Force -Path "$SRC_PATH") > $null
  (New-Item -ItemType Directory -Force -Path "$INCLUDE_PATH") > $null

  # install `.\build\uv\src`
  Copy-Item -Path "$WORKING_BUILD_PATH\libuv\src" -Destination "$ASSET_PATH\uv\src" -Recurse -Force

  # install `.\src\*`
  Copy-Item -Path "$WORKING_PATH\src\*" -Destination "$SRC_PATH" -Recurse -Force -Container

  # install `.\build\include\*`
  Copy-Item -Path "$WORKING_BUILD_PATH\include\*" -Destination "$INCLUDE_PATH" -Recurse -Force -Container

  # install `.\build\lib\*`
  Copy-Item -Path "$WORKING_BUILD_PATH\lib\*" -Destination "$LIB_PATH" -Recurse -Force -Container

  # Remove .pdb file if not debug build.
  if (($debug -ne $true) -and  (Test-Path -Path "$LIB_PATH\uv_a.pdb" -PathType Leaf)) {
    Remove-Item -Path "$LIB_PATH\uv_a.pdb"
  }

  # install `.\build\bin\*`
  Copy-Item -Path "$WORKING_BUILD_PATH\bin\*" -Destination "$BIN_PATH"

  Write-Output "ok - installed files to '$ASSET_PATH'."
}

#
# Get and extract just the WebView2 files that we need
#
Function Install-WebView2 {
  Write-Output "# setting up WebView2"

  $tmpdir = Join-Path $Env:Temp $(New-Guid)
  $base = "$tmpdir\WebView2\build\native"

  # ensure paths
  (New-Item -Type Directory -Path $tmpdir) > $null
  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\lib") > $null
  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\include") > $null

  # download and extract
  Write-Output "# downloading WebView2 ${WEBVIEW2_VERSION} header and library files..."
  Invoke-WebRequest "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}" -O "$tmpdir\webview2.zip"
  Expand-Archive -Path $tmpdir\WebView2.zip -DestinationPath $tmpdir\WebView2

  # install files into project `lib\` dir
  Write-Output "# installing latest WebView2 header and library files..."

  Copy-Item -Path $base\include\WebView2.h "$WORKING_BUILD_PATH\include\WebView2.h" -Force
  Copy-Item -Path $base\include\WebView2Experimental.h "$WORKING_BUILD_PATH\include\WebView2Experimental.h" -Force
  Copy-Item -Path $base\include\WebView2EnvironmentOptions.h "$WORKING_BUILD_PATH\include\WebView2EnvironmentOptions.h" -Force
  Copy-Item -Path $base\include\WebView2ExperimentalEnvironmentOptions.h "$WORKING_BUILD_PATH\include\WebView2ExperimentalEnvironmentOptions.h" -Force
  Copy-Item -Path $base\x64\WebView2LoaderStatic.lib "$WORKING_BUILD_PATH\lib\WebView2LoaderStatic.lib" -Force

  Write-Output "ok - updated WebView2 header files..."
}

# download and install `choco.exe` if it isn't installed yet
(Get-Command "choco.exe" -ErrorAction SilentlyContinue) > $null
if ($? -ne 1) {
  $InstallDir='C:\ProgramData\chocoportable'
  $env:ChocolateyInstall="$InstallDir"

  Set-ExecutionPolicy Bypass -Scope Process -Force;
  iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
}

# setup `clang++.exe` from `llvm` if not installed yet
(Get-Command "clang++.exe" -ErrorAction SilentlyContinue) > $null
if ($? -ne 1) {
  choco install llvm --confirm --force

  if ($? -ne 1) {
    Write-Output "not ok - unable to install llvm"
    Exit 1
  }
}

# install `cmake.exe`
(Get-Command "cmake.exe" -ErrorAction SilentlyContinue) > $null
if ($? -ne 1) {
  choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' --confirm --force

  if ($? -ne 1) {
    Write-Output "not ok - unable to install cmake"
    Exit 1
  }
}

# install `git.exe`
(Get-Command "git.exe" -ErrorAction SilentlyContinue) > $null
if ($? -ne 1) {
  choco install git

  if ($? -ne 1) {
    Write-Output "not ok - unable to install git"
    Exit 1
  }
}

# refresh enviroment after prereq setup
refreshenv
if (-not (Test-Path -Path $ASSET_PATH)) {
  (New-Item -ItemType Directory -Path $ASSET_PATH) > $null
  Write-Output "ok - created $ASSET_PATH"
}

Write-Output "# working path set to $WORKING_PATH"
cd $WORKING_PATH

if ($skipwebview -eq $false) {
  Install-WebView2
}
Build
Install-Files

cd $OLD_CWD
