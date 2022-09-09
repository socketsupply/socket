$OLD_CWD = (Get-Location).Path

$TEMP_PATH = Join-Path $Env:Temp $(New-Guid)
(New-Item -Type Directory -Path $TEMP_PATH) > $null

$SRC_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\src"
$LIB_PATH = "$env:LOCALAPPDATA\Programs\socketsupply\lib"
$WORKING_PATH = $OLD_CWD

if (Test-Path -Path $SRC_PATH) {
  Remove-Item -Recurse -Force $SRC_PATH
  Write-Output "ok - cleaned $SRC_PATH"
}

if (Test-Path -Path $LIB_PATH) {
  Remove-Item -Recurse -Force $LIB_PATH
  Write-Output "ok - cleaned $LIB_PATH"
}

(New-Item -ItemType Directory -Force -Path $SRC_PATH) > $null
Write-Output "ok - created $SRC_PATH"

(New-Item -ItemType Directory -Force -Path $LIB_PATH) > $null
Write-Output "ok - created $LIB_PATH"

#
# Compile with the current git revision of the repository
#
Function Build {
  $VERSION_HASH = $(git rev-parse --short HEAD) 2>&1 | % ToString
  $VERSION = $(type VERSION.txt) 2>&1 | % ToString
  $BUILD_TIME = [int] (New-TimeSpan -Start (Get-Date "01/01/1970") -End (Get-Date)).TotalSeconds
  $BUILD_PATH = "$WORKING_PATH\build"

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
    Write-Output "ok - cpmmand ssc has been added to the path for the current session."
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
  Copy-Item -Path "$WORKING_PATH\src\*" -Destination $SRC_PATH -Recurse
  Copy-Item -Path "$WORKING_PATH\src\win64\*" -Destination $SRC_PATH -Recurse
  Write-Output "ok - installed files to '$SRC_PATH'."
}

Write-Output "# working path set to $WORKING_PATH"

if ($args[0] -eq "update") {
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

  Exit 0
}


(Get-Command cmake.exe) > $null

if ($? -ne 1) {
  if ($env:PROCESSOR_ARCHITECTURE -eq '*64*') {
    Write-Output "# installing cmake..."
    Invoke-WebRequest https://github.com/Kitware/CMake/releases/download/v3.24.1/cmake-3.24.1-windows-x86_64.msi -O cmake.msi
    cmake.msi
  } else {
    Write-Output "not ok - cmake required (try 'choco install cmake')"
    Exit
  }
}

(Get-Command clang++) > $null

if ($? -ne 1) {
  if ($env:PROCESSOR_ARCHITECTURE -eq '*64*') {
    Write-Output "# installing llvm..."
    Invoke-WebRequest https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.0/LLVM-15.0.0-win64.exe -O llvm.exe
    llvm.exe
  } else {
    Write-Output "not ok - llvm toolchain required (try 'choco install llvm')"
    Exit
  }
}

(Get-Command git) > $null

if ($? -ne 1) {
  Write-Output "not ok - git is not installed"
  Exit 1
}

cd $WORKING_PATH

Build
Install-Files

cd $OLD_CWD
