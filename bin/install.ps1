param([Switch]$debug, [Switch]$verbose, [Switch]$force, [Switch]$shbuild=$true, $toolchain = "vsbuild")

# -shbuild:$false - Don't run bin\install.sh (Builds runtime lib)
# -debug          - Enable debug builds (DEBUG=1)
# -verbose        - Enable verbose build output

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
$global:git = "git.exe"
$global:cmake = "cmake.exe"
$global:useCurl = $true

if ($debug -eq $true) {
  $LIBUV_BUILD_TYPE = "Debug"
  
  $SSC_BUILD_OPTIONS = "-g", "-O0"
  [Environment]::SetEnvironmentVariable("DEBUG", "1")
}

if ($verbose -eq $true) {
  [Environment]::SetEnvironmentVariable("VERBOSE", "1")
}

if ($force -eq $true) {
  $global:forceArg = "--force"
}

$global:path_advice = @()
$vsconfig = "nmake.vsconfig"

if ( -not (("llvm+vsbuild" -eq $toolchain) -or ("vsbuild" -eq $toolchain) -or ("llvm" -eq $toolchain)) ) {
  Write-Output "Unsupported -toolchain $toolchain. Supported options are vsbuild, llvm+vsbuild or llvm (external nmake required)"
  Write-Output "-toolchain llvm+vsbuild will check for and install llvm clang and vsbuild nmake."
  Exit 1
}

if ("vsbuild" -eq $toolchain) {
  if ($shbuild) {
    $vsconfig = ".vsconfig"
  } else {
    # Use smaller footprint if we're not doing an install.sh
    $vsconfig = ".vsconfig-app-build"
  }
}


Write-Output "Using toolchain: $toolchain"

Function Found-Command {
    param($command_string)
    (Get-Command $command_string -ErrorAction SilentlyContinue -ErrorVariable F) > $null
    $r = $($null -eq $F.length)
    Write-Output $r
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

$bin = "bin"
if ((Test-Path "bin" -PathType Container) -eq $false) {
  . .\Get-VCVars.ps1
  . .\Get-ProcEnvs.ps1
  $bin = ""
} else {
  . .\bin\Get-VCVars.ps1
  . .\bin\Get-ProcEnvs.ps1
}

Function Get-UrlCall {
  param($url, $dest)
  # curl is way faster than iwr
  # need .exe because curl is an alias for iwr, go figure...  
  if ($global:useCurl) {
    return "iex ""& curl.exe -L """"$url"""" --output """"$dest"""""""
  } else {
    return "iwr $url -O $dest"
  }
}

Function Install-Requirements {

  if (-not (Found-Command("curl"))) {
    $global:useCurl = $false
  }

  $install_tasks = @()
  
  $vc_runtime_test_path = "$env:SYSTEMROOT\System32\vcruntime140_1.dll"
  if ((Test-Path "$vc_runtime_test_path" -PathType Leaf) -eq $false) {
    
    $installer = "vc_redist.x64.exe"
    $url = "https://aka.ms/vs/17/release/$installer"
    $confirmation = Read-Host "$installer is a requirement, proceed with install from Microsoft? y/[n]?" 
    
    if ($confirmation -eq 'y') {
      $t = [string]{
        Write-Output "Downloading $url"
        -geturl-
        Write-Output "Installing $env:TEMP\$installer"
        iex "& $env:TEMP\$installer /quiet"
      }
      $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
      $install_tasks += $t
    }
  } else {
    Write-Output "# $vc_runtime_test_path found."
  }

  $gitPath = "$env:ProgramFiles\Git\bin"

  # Look for git in path
  if (-not (Found-Command($global:git))) {
    # Look for git in default location, in case it was installed in a previous session
    $global:git = "$gitPath\$global:git"
    $global:path_advice += "SET PATH=""$gitPath"";%PATH%"
  } else {
    Write-Output ("Git found at, changing path to: $global:git")
  }

  if (-not (Found-Command($global:git))) {

    $confirmation = Read-Host "git is a requirement, proceed with install from github.com/git-for-windows? y/[n]?"
    $installer = "Git-2.39.1-64-bit.exe"
    $installer_tmp = "Git-2.39.1-64-bit.tmp"
    $url = "https://github.com/git-for-windows/git/releases/download/v2.39.1.windows.1/$installer"

    if ($confirmation -eq 'y') {
      $t = [string]{
        Write-Output "Downloading $url"
        -geturl-
        Write-Output "Installing $env:TEMP\$installer"
        iex "& $env:TEMP\$installer /SILENT"
        sleep 1
        $proc=Get-Process $installer_tmp
        Write-Output "Waiting for $installer_tmp..."
        Wait-Process -InputObject $proc
      }      
      $t = $t.replace("`$installer_tmp", $installer_tmp).replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
      $install_tasks += $t
    }
  } else {
    Write-Output("# Found git.exe")
  }

  # install `cmake.exe`
  if ($shbuild) {
    $cmakePath = "$env:ProgramFiles\CMake\bin"
    if (-not (Found-Command($global:cmake))) {
      $global:cmake = "$cmakePath\$global:cmake"
      $global:path_advice += "SET PATH=""$cmakePath"";%PATH%"
    }

    if (-not (Found-Command($global:cmake))) {

      $confirmation = Read-Host "CMake is a requirement, proceed with install from cmake.org? y/[n]?"
      $installer = "cmake-3.26.0-rc2-windows-x86_64.msi"
      $url = "https://github.com/Kitware/CMake/releases/download/v3.26.0-rc2/$installer"

      if ($confirmation -eq 'y') {
        $t = [string]{
          Write-Output "Downloading $url"
          -geturl-
          Write-Output "Installing $env:TEMP\$installer"
          sleep 2
          $proc=Get-Process msiexec
          Write-Output "Waiting for msiexec..."
          iex "& $env:TEMP\$installer /quiet"
        }
        $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
        $install_tasks += $t
      }
    }
  }

  $clang = "clang++"
  if (("llvm+vsbuild" -eq $toolchain) -or ("llvm" -eq $toolchain)) {
    $clangPath = "$env:ProgramFiles\LLVM\bin"

    if (-not (Found-Command($clang))) {
      $clang = "$clangPath\$clang"
      $global:path_advice += $clangPath
      $global:path_advice += "SET PATH=""$clangPath"";%PATH%"
    }

    if (-not (Found-Command($clang))) {

      $confirmation = Read-Host "LLVM will be downloaded for clang++, proceed? y/[n]?"
      if ($confirmation -eq 'y') {
        $installer = "LLVM-15.0.7-win64.exe"
        $url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/$installer"

        if ($confirmation -eq 'y') {
          $t = [string]{
            Write-Output "Downloading $url"
            -geturl-
            Write-Output "Installing $env:TEMP\$installer"
            iex "& $env:TEMP\$installer /S"
            sleep 1
            $proc=Get-Process $installer
            Write-Output "Waiting for $installer..."
          }
          $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
          $install_tasks += $t
        }
      }

      $env:PATH="$clangPath;$env:PATH"
    } else {
      Write-Output("# Found clang++.exe")
    }
  }

  $and_nmake = ""
  if ($shbuild) {
    $and_nmake = " and nmake"
  }

  if (("llvm+vsbuild" -eq $toolchain) -or ("vsbuild" -eq $toolchain)) {
    $vc_exists, $vc_vars = $(Get-VCVars)
    $report_vc_vars_reqd = $false
    $install_vc_build = $true

    if ($shbuild -and $(Found-Command("clang++.exe")) -and $(Found-Command("nmake.exe"))) {
      Write-Output("# Found clang$and_nmake")
      $install_vc_build = $false
    } elseif ($(Found-Command("clang++.exe"))) {
      Write-Output("# Found clang")
      $install_vc_build = $false
    } else {
      if ($vc_exists) {
        Write-Output "Calling vcvars64.bat"
        $(Get-ProcEnvs($vc_vars))
        if ($shbuild -and $(Found-Command("clang++.exe")) -and $(Found-Command("nmake.exe"))) {
          $report_vc_vars_reqd = $true
          $install_vc_build = $false
        } elseif ($(Found-Command("clang++.exe"))) {
          $report_vc_vars_reqd = $true
          $install_vc_build = $false
        } else {
          Write-Output("# $vc_vars didn't enable both clang++$and_nmake, downloading vs_build.exe")
        }
      }
    }

    if ($install_vc_build) {
      $report_vc_vars_reqd = $true
      $confirmation = Read-Host "Visual Studio Clang, Windows SDK$and_nmake are required, proceed with install from Microsoft? y/[n]?"
      $installer = "vs_buildtools.exe"
      $url = "https://aka.ms/vs/17/release/$installer"

      if ($confirmation -eq 'y') {
        $t = [string]{
          Write-Output "Downloading $url"
          -geturl-
          Write-Output "Installing $env:TEMP\$installer"
          iex "& $env:TEMP\$installer --passive --config $OLD_CWD\$bin\$vsconfig"
        }
        $t = $t.replace("`$OLD_CWD", $OLD_CWD).replace("`$bin", $bin).replace("`$vsconfig", $vsconfig)
        $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
        $install_tasks += $t
      }
    }
  }

  if ($install_tasks.Count -gt 0) {
    Write-Output "Installing build dependencies..."
    $script = ""
    # concatinate all script blocks so a single UAC request is raised
    foreach ($task in $install_tasks) {
      $script = "$($script)Invoke-Command {$($task)}`r`n"
    }

    try {
      Start-Process powershell -verb runas -wait -ArgumentList "$script"
    } catch [InvalidOperationException] {
      Write-Output "UAC declined, nothing installed."
      Exit 1
    }
  }

  if ($install_vc_build) {
    $vc_exists, $vc_vars = $(Get-VCVars)
    if ($vc_exists) {
      $(Get-ProcEnvs($vc_vars))
    } else {
      Write-Output "vcvars64.bat still not present, something went wrong."
      Exit 1
    }
  }

  if (-not (Found-Command($clang))) {
    Write-Output "not ok - unable to install clang++."
    Exit 1
  }

  if ((Test-Path "$vc_runtime_test_path" -PathType Leaf) -eq $false) {
    Write-Output "$vc_runtime_test_path still not present, something went wrong."
    Exit 1
  }

  if (-not (Found-Command($global:git))) {
    Write-Output "not ok - unable to install git."
    Exit 1
  }

  if ($shbuild) {
    if (-not (Found-Command($global:cmake))) {
      Write-Output "not ok - unable to install cmake"
      Exit 1
    }
  }

  if ($report_vc_vars_reqd) {
    $global:path_advice += """$vc_vars"""
  }

  # refresh enviroment after prereq setup
  if ($(Found-Command("refreshenv"))) {
    refreshenv
  }
}

Install-Requirements

if ($shbuild) {
  $gitPath = "$env:ProgramFiles\Git\bin"
  $sh = "$gitPath\sh.exe"

  if ($env:VERBOSE -eq "1") {
    Write-Output "Using shell $sh"
    iex "& ""$sh"" -c 'uname -s'"
  }

  # Look for sh in path
  if (-not (Found-Command($sh))) {
    $sh = "$gitPath\sh.exe"
    Write-Output "sh.exe not in PATH or default Git\bin"
    Exit 1
  }

  $find_check = iex "& ""$sh"" -c 'find --version'" | Out-String

  if (-not ($find_check -like "*find (GNU findutils)*")) {
    Write-Output "find is not GNU findutils: '$find_check'"
    Exit 1
  }

  cd $OLD_CWD
  Write-Output "Calling bin\install.sh $forceArg"
  iex "& ""$sh"" bin\install.sh $forceArg"
}

if ($global:path_advice.Count -gt 0) {
  Write-Output "Please add the following to PATH or run in future dev sessions: "
  foreach ($p in $global:path_advice) {
    Write-Output $p
  }
}

cd $OLD_CWD
