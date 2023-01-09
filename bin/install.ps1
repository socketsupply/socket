param([Switch]$debug, [Switch]$skipwebview, $webview = "1.0.1619-prerelease", $uv = "v1.44.2", $toolchain = "vsbuild")

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
$git = "git.exe"
$cmake = "cmake.exe"

if ($debug -eq $true) {
  $LIBUV_BUILD_TYPE = "Debug"
  $SSC_BUILD_OPTIONS = "-g", "-O0"
}

$path_advice = @()
$vsconfig = "nmake.vsconfig"

if ( -not (("llvm+vsbuild" -eq $toolchain) -or ("vsbuild" -eq $toolchain) -or ("llvm" -eq $toolchain)) ) {
  Write-Output "Unsupported -toolchain $toolchain. Supported options are vsbuild, llvm+vsbuild or llvm (external nmake required)"
  Write-Output "-toolchain llvm+vsbuild will check for and install llvm clang and vsbuild nmake."
  Exit 1
}

if ("vsbuild" -eq $toolchain) {
  $vsconfig = ".vsconfig"
}

Write-Output "Using toolchain: $toolchain"

Function Found-Command {
    param($command_string)
    (Get-Command $command_string -ErrorAction SilentlyContinue -ErrorVariable F) > $null
    # Write-Output ">$command_string< : $($F.length), $F"
    # $r = $($error.length -eq 0)
    $r = $($null -eq $F.length)
    # Write-Output "Return $($F.length), $r"
    Write-Output $r
}

#
# Compile with the current git revision of the repository
#
Function Build {
  $VERSION_HASH = $(git rev-parse --short=8 HEAD) 2>&1 | % ToString
  $VERSION = $(type VERSION.txt) 2>&1 | % ToString
  $BUILD_TIME = [int] (New-TimeSpan -Start (Get-Date "01/01/1970") -End (Get-Date)).TotalSeconds

  if (-not (Test-Path -Path "$WORKING_BUILD_PATH\libuv" -PathType Container)) {
    (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH") > $null
    Write-Output "# cloning libuv at $LIBUV_TAG from github..."
    # Use iex "& " syntax so we can use $git variable, send output to variable so it doesn't display on screen
    $s = iex "& ""$git"" clone --branch=$LIBUV_TAG -q --depth=1 https://github.com/libuv/libuv.git $WORKING_BUILD_PATH\libuv 2>&1"
    # ( """$git"" clone --branch=$LIBUV_TAG -q --depth=1 https://github.com/libuv/libuv.git $WORKING_BUILD_PATH\libuv") > $null 2> $null
    if (-not (Test-Path -Path "$WORKING_BUILD_PATH\libuv\.git" -PathType Container)) {
      Write-Output "Failed to clone libuv"
      Exit 1
    } else {
      Write-Output "ok - cloned libuv into $WORKING_BUILD_PATH\libuv"
    }
  }

  (New-Item -ItemType Directory -Force -Path "$WORKING_BUILD_PATH\libuv\build") > $null

  Write-Output "# building libuv..."
  cd "$WORKING_BUILD_PATH\libuv\build"
  (iex "& ""$cmake"" .. -DBUILD_TESTING=OFF") > $null

  cd "$WORKING_BUILD_PATH\libuv"
  (iex "& ""$cmake"" --build ""$WORKING_BUILD_PATH\libuv\build"" --config $LIBUV_BUILD_TYPE") > $null
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
$ChocoDir=$env:ChocolateyInstall
$choco = "$ChocoDir\bin\choco.exe"
if (-not (Found-Command("$choco"))) {
  # Look for choco in standard location just in case, otherwise set up var $env
  $ChocoDir="$($env:ProgramData)\chocolatey"
  $choco = "$ChocoDir\bin\choco.exe"
}

if (-not (Found-Command("$choco"))) {  
  $env:ChocolateyInstall="$ChocoDir"

  Set-ExecutionPolicy Bypass -Scope Process -Force;
  Start-Process powershell -verb runas -wait -ArgumentList "iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
  
} else {
  Write-Output "# chocolatey is installed"
}

if (("llvm+vsbuild" -eq $toolchain) -or ("llvm" -eq $toolchain))
{
  $clang = "clang++"
  $clangPath = "$env:ProgramFiles\LLVM\bin"

  if (-not (Found-Command($clang))) {
    $clang = "$clangPath\$clang"
    $path_advice += $clangPath
  }

  if (-not (Found-Command($clang))) {

    # Note that this installs git with LFS
    Start-Process $choco -verb runas -wait -ArgumentList "install","llvm","--confirm","--force" > $null
    $env:PATH="$clangPath;$env:PATH"

    if (-not (Found-Command($clang))) {
      Write-Output "not ok - unable to install clang++."
      Exit 1
    }
  } else {
    Write-Output("Found clang++.exe")
  }
}

if (("llvm+vsbuild" -eq $toolchain) -or ("vsbuild" -eq $toolchain))
{
  $bin = "bin"
  if ((Test-Path "bin" -PathType Container) -eq $false) {
    . .\Get-VCVars.ps1
    . .\Get-ProcEnvs.ps1
    $bin = ""
  } else {
    . .\bin\Get-VCVars.ps1
    . .\bin\Get-ProcEnvs.ps1
  }

  $vc_exists, $vc_vars = $(Get-VCVars)
  $report_vc_vars_reqd = $false
  $install_vc_build = $true

  if ($(Found-Command("clang++.exe")) -and $(Found-Command("nmake.exe"))) {
    Write-Output("Found clang and nmake")
    $install_vc_build = $false
  } else {
    if ($vc_exists) {
      Write-Output "Calling vcvars64.bat"
      $(Get-ProcEnvs($vc_vars))
      if ($(Found-Command("clang++.exe")) -and $(Found-Command("nmake.exe"))) {
        $report_vc_vars_reqd = $true
        $install_vc_build = $false
      } else {
        Write-Output("# $vc_vars didn't enable both clang++ and nmake, downloading vs_build.exe")
      }
    } else {
      Write-Output("# Visual Studio Build Tools not detected, please install all selected items.")
    }
  }

  if ($install_vc_build) {
    $report_vc_vars_reqd = $true
    $tmpdir = Join-Path $Env:Temp $(New-Guid)
    (New-Item -Type Directory -Path $tmpdir) > $null
    if ($null -ne ($env:VSFROMLAYOUT)) {
      Write-Output "# Installing Visual Studio Build Tools from pre downloaded layout folder $($env:VSFROMLAYOUT)"
      cd $($env:VSFROMLAYOUT)
      Start-Process "vs_setup.exe" -verb runas -wait -ArgumentList "-P" > $null
      cd $OLD_CWD
    } else {
      Invoke-WebRequest "https://aka.ms/vs/17/release/vs_buildtools.exe" -O "$tmpdir\vs_buildtools.exe"
      Start-Process "$tmpdir\vs_buildtools.exe" -verb runas -wait -ArgumentList "--config","$OLD_CWD\$bin\$vsconfig" > $null
    }

    $vc_exists, $vc_vars = $(Get-VCVars)
    if ($vc_exists) {
      Write-Output "Calling vcvars64.bat"
      $(Get-ProcEnvs($vc_vars))
    } else {
      Write-Output "vcvars64.bat still not present, something went wrong."
      Exit 1
    }
  }

  if ($report_vc_vars_reqd) {
    Write-Output "vcvars64.bat found and enabled clang++. Note that you will need to call it manually if running this script from cmd.exe under a separate powershell request:"
    $path_advice += $vc_vars
    Write-Output $vc_vars
  }
}

$gitPath = "$env:ProgramFiles\Git\bin"

# Look for git in path
if (-not (Found-Command($git))) {
  # Look for git in default location, in case it was installed in a previous session
  $git = "$gitPath\$git"
  $path_advice += $gitPath
}

if (-not (Found-Command($git))) {

  # Note that this installs git with LFS
  Start-Process $choco -verb runas -wait -ArgumentList "install","-y","git" > $null

  if (-not (Found-Command($git))) {
    Write-Output "not ok - unable to install git, you could try running: "
    Write-Output "%programdata%\chocolatey\bin\choco uninstall -force -y git"
    Exit 1
  }
} else {
  Write-Output("Found git.exe")
}

# install `cmake.exe`
$cmakePath = "$env:ProgramFiles\CMake\bin"
if (-not (Found-Command($cmake))) {
  $cmake = "$cmakePath\$cmake"
  $path_advice += $cmakePath
}

if (-not (Found-Command($cmake))) {
  Start-Process $choco -verb runas -wait -ArgumentList "install","-y","cmake" > $null

  if (-not (Found-Command($cmake))) {
    Write-Output "not ok - unable to install cmake"
    Exit 1
  }
}


# refresh enviroment after prereq setup
if ($(Found-Command("refreshenv"))) {
  refreshenv
}

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

if ($path_advice.Count -gt 0) {
  Write-Output "Please add the following to PATH or run for this and future dev sessions: "
  foreach ($p in $path_advice) {
    Write-Output $p
  }
}

cd $OLD_CWD
