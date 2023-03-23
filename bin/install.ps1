param([Switch]$debug, [Switch]$verbose, [Switch]$force, [Switch]$shbuild=$true, [Switch]$package_setup, $toolchain = "vsbuild")

# -shbuild:$false - Don't run bin\install.sh (Builds runtime lib)
#                 - Fail silently (allow npm install-pre-reqs script '&& exit' to work to close y/n prompt window)
# -debug          - Enable debug builds (DEBUG=1)
# -verbose        - Enable verbose build output (VERBOSE=1)

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
$global:WIN_DEBUG_LIBS = ""
$global:path_advice = @()
$global:install_errors = @()
$global:forceArg = ""
$targetClangVersion = "15.0.0"
$targetCmakeVersion = "3.24.0"
$logfile="$env:LOCALAPPDATA\socket_install_ps1.log"

# Powershell environment variables are maintained across sessions, clear if not explicitly set
$set_debug=$null
if ($debug) {
  $LIBUV_BUILD_TYPE = "Debug"
  
  $SSC_BUILD_OPTIONS = "-g", "-O0"
  $set_debug="1"
}
[Environment]::SetEnvironmentVariable("DEBUG", $set_debug)

$set_verbose=$null
if ($verbose) {
  $set_verbose="1"
}
[Environment]::SetEnvironmentVariable("VERBOSE", $set_verbose)

if ($force -eq $true) {
  $global:forceArg = "--force"
}

Function Exit-IfErrors {
  if ($global:install_errors.Count -gt 0) {
    foreach ($e in $global:install_errors) {
      Write-Log "h" $e
    }
    Exit $global:install_errors.Count
  }
}

function Call-VCVars() {
  Write-Log "h" "# Calling vcvars64.bat"
  $new_envs = $(Get-ProcEnvs("$OLD_CWD", $vc_vars))
  if ($debug) {
    foreach ($s in $new_envs) {
      Write-Log "d" "$s"
    }
    Dump-VsEnvVars
  }
  Write-Output "Errors: " 
  if ($new_envs[0] -ne "0") {
    $global:install_errors += "not ok - vcvars64.bat failed ($($new_envs[0])); Please review $logfile and submit a bug if you think there's an issue."
    Exit-IfErrors
  }
}

# type: 
# d - debug / console / file
# v - verbose / console / file
# h - host (console) / file
# f - file
function Write-Log ($type, $message) {
  $wh = $false
  $wf = $false
  if (($debug) -and (($type -eq "d") -or ($type -eq "v"))) {
    $wf = $wh = $true
  } elseif (($verbose) -and ($type -eq "v")) {
    $wf = $wh = $true
  } elseif ($type -eq "f") {
    # f doesn't write to host
  } elseif ($type -eq "h") {    
    $wh = $true
  }

  if ($wh) {
    Write-Host $message
  }

  Write-LogFile $message

}

function Dump-VsEnvVars() {
  $path = $env:PATH.split(";")
  $msvs="Microsoft Visual Studio"
  Write-Log "d" "WindowsSDKLibVersion: $env:WindowsSDKLibVersion"
  Write-Log "d" "Searching in path for $msvs"
  foreach ($s in $env:PATH.split(";")) {
    if ($s -like "*$msvs*") {
      Write-Log "d" "$s"
    }
  }
}

function Write-LogFile($message)
{
  Write-Output $message >> $logfile
}

Function Prompt {
  param($text)
  Write-LogFile "$text`n> "
  Write-Host "$text`n> " -NoNewLine
  $r = $Host.UI.ReadLine()
  Write-LogFile "User input: $r"
  Write-Output $r
}

Function Get-CommandPath {
  param($command_string)
    $c = (Get-Command "$command_string" -ErrorAction SilentlyContinue -ErrorVariable F).Source
    $r = $($null -eq $F.length)
    if ($r -eq $true) {
      Write-Output $c
      return
    }
    Write-Output $r
}

Function Found-Command {
    param($command_string)
    (Get-Command $command_string -ErrorAction SilentlyContinue -ErrorVariable F) > $null
    $r = $($null -eq $F.length)
    Write-Output $r
}

Function Test-CommandVersion {
  param($params)
  $command_string = $params[0]
  $target_version = $params[1]
  $debug = $params.Count -gt 2

  Write-Log "d" "Test-CommandVersion: $command_string $target_version"

  $fso = New-Object -ComObject Scripting.FileSystemObject

  while ($true) {
    (Get-Command $command_string -ErrorAction SilentlyContinue -ErrorVariable F) > $null
    $r = $($null -eq $F.length)
    if ($r -eq $false) {
      Write-Log "d" "Get-Command: Command not found"
      Write-Output $r
      return
    }

    $output = iex "& $command_string --version" | Out-String
    $output = $output.split("`r`n")[0].split(" ")

    for (($i = 0); ($i -lt $output.Count); ($i++)) {
      if ($output[$i] -eq "version") {
        $current_version = $output[$i+1]
      }
    }

    $current_version = $current_version.split("-")[0]

    Write-Log "v" "Current $($fso.GetFile($(Get-CommandPath $command_string)).Path): $current_version, target: $target_version"

    $ta = @()
    foreach ($v in $target_version.split(".")) {
      $ta += [int]$v
    }

    $current_version_split = $current_version.split(".")

    $ca = @()
    # Ignore invalid (not equal) version strings
    if ($current_version_split.Count -eq $ta.Count) {
      foreach ($v in $current_version_split) {
        $ca += [int]$v
      }
    } elseif ($debug) {
      Write-Log "d" "($current_version_split).Count <> ($ta).Count}"
      Write-Output $false
      return
    }

    if ($ca.Count -ne $ta.Count) {
      Write-Log "d" "$($ca.Count) <> $($ta.Count)" 
      Write-Output "$($ca.Count) <> $($ta.Count)"
      Write-Output $false
    }

    for (($i = 0); ($i -lt $ca.Count); ($i++)) {
      # Current element is lower, no point in comparing other elements
      if ($ca[$i] -lt $ta[$i]) {
        break;
      }

      # Current element is greater, no need to compare other elements
      if ($ca[$i] -gt $ta[$i]) {
        Write-Log "d" "$current_version >= $target_version"
        Write-Output $true
        return
      }

      # Current element is equal, test remaining elements
    }

    # Remove current item's path so it isn't used in future searches
    $p = $fso.GetFile($(Get-CommandPath $command_string)).ParentFolder.Path
    Write-Log "d" "Remove $p from PATH"
    $env:PATH = $env:PATH.replace("$p", "")
  }

  Write-Log "d" "No viable versions of $command_string"
  Write-Output $false
}

$vsconfig = "nmake.vsconfig"

if ( -not (("llvm+vsbuild" -eq $toolchain) -or ("vsbuild" -eq $toolchain) -or ("llvm" -eq $toolchain)) ) {
  Write-Log "h" "not ok - Unsupported -toolchain $toolchain. Supported options are vsbuild, llvm+vsbuild or llvm (external nmake required)"
  Write-Log "h" "not ok - -toolchain llvm+vsbuild will check for and install llvm clang $targetClangVersion and vsbuild nmake."

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


Write-Log "h" "# Using toolchain: $toolchain"

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

  # install `.\build\bin\*`
  Copy-Item -Path "$WORKING_BUILD_PATH\bin\*" -Destination "$BIN_PATH"

  Write-Log "h" "ok - installed files to '$ASSET_PATH'."
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
  Write-Log "v" "# Logging to $logfile"
  if (-not (Found-Command("curl"))) {
    $global:useCurl = $false
  }

  $install_tasks = @()
  
  $vc_runtime_test_path = "$env:SYSTEMROOT\System32\vcruntime140_1.dll"
  if ((Test-Path "$vc_runtime_test_path" -PathType Leaf) -eq $false) {
    
    $installer = "vc_redist.x64.exe"
    $url = "https://aka.ms/vs/17/release/$installer"
    $confirmation = Prompt "$installer is a requirement, proceed with install from Microsoft? y/[n]?" 
    
    if ($confirmation -eq 'y') {
      $t = [string]{
        Write-Output "# Downloading $url"
        -geturl-
        Write-Output "# Installing $env:TEMP\$installer"
        iex "& $env:TEMP\$installer /quiet"
      }
      $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
      $install_tasks += $t
    }
  } else {
    Write-Log "h" "# $vc_runtime_test_path found."
  }

  $gitPath = "$env:ProgramFiles\Git\bin"

  # Look for git in path
  if (-not (Found-Command($global:git))) {
    # Look for git in default location, in case it was installed in a previous session
    $global:git = "$gitPath\$global:git"
    $global:path_advice += "`$env:PATH='$gitPath;'+`$env:PATH"
  } else {
    Write-Output ("# Git found at, changing path to: $global:git")
  }

  if (-not (Found-Command($global:git))) {

    $confirmation = Prompt "git is a requirement, proceed with install from github.com/git-for-windows? y/[n]?"
    $installer = "Git-2.39.1-64-bit.exe"
    $installer_tmp = "Git-2.39.1-64-bit.tmp"
    $url = "https://github.com/git-for-windows/git/releases/download/v2.39.1.windows.1/$installer"

    if ($confirmation -eq 'y') {
      $t = [string]{
        Write-Output "# Downloading $url"
        -geturl-
        Write-Output "# Installing $env:TEMP\$installer"
        iex "& $env:TEMP\$installer /SILENT"
        sleep 1
        $proc=Get-Process $installer_tmp
        Write-Output "# Waiting for $installer_tmp..."
        Wait-Process -InputObject $proc
      }      
      $t = $t.replace("`$installer_tmp", $installer_tmp).replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
      $install_tasks += $t
    }
  } else {
    Write-Log "h" "# Found git.exe"
  }

  # install `cmake.exe`
  if ($shbuild) {
    $cmakePath = ""
    if (-not (Test-CommandVersion("cmake", $targetCmakeVersion))) {
      $cmakePath = "$env:ProgramFiles\CMake\bin"
      if ((Test-Path $cmakePath -PathType Container) -eq $true) {
        $env:PATH="$cmakePath\;$env:PATH"
        $global:cmake = "$cmakePath\$global:cmake"
      }
    }

    if (-not (Test-CommandVersion("cmake", $targetCmakeVersion))) {

      $confirmation = Prompt "CMake is a requirement, proceed with install from cmake.org? y/[n]?"
      $installer = "cmake-3.26.0-rc2-windows-x86_64.msi"
      $url = "https://github.com/Kitware/CMake/releases/download/v3.26.0-rc2/$installer"

      if ($confirmation -eq 'y') {
        $t = [string]{
          Write-Output "# Downloading $url"
          -geturl-
          Write-Output "# Installing $env:TEMP\$installer"
          sleep 2
          $proc=Get-Process msiexec
          Write-Output "# Waiting for msiexec..."
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

    if (-not (Test-CommandVersion("clang++", $targetClangVersion))) {
      $clang = "$clangPath\$clang"
      $global:path_advice += "`$env:PATH='$clangPath;'+`$env:PATH"
    }

    if (-not (Test-CommandVersion("clang++", $targetClangVersion))) {

      $confirmation = Prompt "LLVM will be downloaded for clang++, proceed? y/[n]?"
      if ($confirmation -eq 'y') {
        $installer = "LLVM-15.0.7-win64.exe"
        $url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/$installer"

        if ($confirmation -eq 'y') {
          $t = [string]{
            Write-Output "# Downloading $url"
            -geturl-
            Write-Output "# Installing $env:TEMP\$installer"
            iex "& $env:TEMP\$installer /S"
            sleep 1
            $proc=Get-Process $installer
            Write-Output "# Waiting for $installer..."
          }
          $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
          $install_tasks += $t
        }
      }

      $env:PATH="$clangPath;$env:PATH"
    } else {
      Write-Log "h" "# Found clang++.exe"
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

    if ($shbuild -and $(Test-CommandVersion("clang++", $targetClangVersion)) -and $(Found-Command("nmake.exe"))) {
      Write-Log "h" "# Found clang$and_nmake"
      $install_vc_build = $false
    } elseif ($(Test-CommandVersion("clang++", $targetClangVersion))) {
      Write-Log "h" "# Found clang"
      $install_vc_build = $false
    } else {
      if ($vc_exists) {
        Call-VcVars
        if ($shbuild -and $(Test-CommandVersion("clang++", $targetClangVersion)) -and $(Found-Command("nmake.exe"))) {
          $report_vc_vars_reqd = $true
          $install_vc_build = $false
        } elseif ($(Test-CommandVersion("clang++", $targetClangVersion))) {
          $report_vc_vars_reqd = $true
          $install_vc_build = $false
        } else {
          Write-Log "h" "# $vc_vars didn't enable both clang++$and_nmake, downloading vs_build.exe"
          Dump-VsEnvVars
        }
      }
    }

    # Check windows SDK
    if (($env:WindowsSdkDir -eq $null) -or ((Test-Path $env:WindowsSdkDir -PathType Container) -eq $false)) {
      $install_vc_build = $true
    }

    if ($install_vc_build) {
      $report_vc_vars_reqd = $true
      $confirmation = Prompt "clang $targetClangVersion, Windows SDK$and_nmake are required, proceed with install from Microsoft? y/[n]?"
      $installer = "vs_buildtools.exe"
      $url = "https://aka.ms/vs/17/release/$installer"

      if ($confirmation -eq 'y') {
        $t = [string]{
          Write-Output "# Downloading $url"
          -geturl-
          Write-Output "# Installing $env:TEMP\$installer"
          iex "& $env:TEMP\$installer --passive --config $OLD_CWD\$bin\$vsconfig"
        }
        $t = $t.replace("`$OLD_CWD", $OLD_CWD).replace("`$bin", $bin).replace("`$vsconfig", $vsconfig)
        $t = $t.replace("`$installer", $installer).replace("`$env:TEMP", $env:TEMP).replace("`$url", $url).replace("-geturl-", (Get-UrlCall "$url" "$env:TEMP\$installer")).replace("""", "\""")
        $install_tasks += $t
      }
    }
  }

  if ($install_tasks.Count -gt 0) {
    Write-Log "h" "# Installing build dependencies..."
    $script = ""
    # concatinate all script blocks so a single UAC request is raised
    foreach ($task in $install_tasks) {
      $script = "$($script)Invoke-Command {$($task)}`r`n"
    }

    try {
      Start-Process powershell -verb runas -wait -ArgumentList "$script"
    } catch [InvalidOperationException] {
      $global:install_errors += "not ok - UAC declined, nothing installed."
    }
  }

  if ($install_vc_build) {
    $vc_exists, $vc_vars = $(Get-VCVars)
    if ($vc_exists) {
      Call-VcVars
    } else {
      if ($env:CI -eq $null) {
        $global:install_errors += "not ok - vcvars64.bat still not present, something went wrong."
      }
    }
  }

  if (-not (Test-CommandVersion("clang++", $targetClangVersion))) {
    $global:install_errors += "not ok - unable to install clang++."
  }

  if ($env:CI -eq $null) {
    if (($env:WindowsSdkDir -eq $null) -or ((Test-Path $env:WindowsSdkDir -PathType Container) -eq $false)) {
      # Had this situation occur after uninstalling SDK from add/remove programs instead of VS Installer.
      $global:install_errors += "`$WindowsSdkDir ($env:WindowsSdkDir) still not present, please install manually."
    } else {
      # Find lib required for debug builds (Prevents 'Debug Assertion Failed. Expression: (_osfile(fh) & fopen)' error)
      $WIN_DEBUG_LIBS="$($env:WindowsSdkDir)Lib\$($env:WindowsSDKLibVersion)ucrt\x64\ucrtd.osmode_permissive.lib"
      if ((Test-Path $WIN_DEBUG_LIBS -PathType Leaf) -eq $false) {
        if ($shbuild -eq $true) {
          # Only report issue for ssc devs
          $global:path_advice += "WARNING: Unable to determine ucrtd.osmode_permissive.lib path. This is only required for DEBUG builds."
        } else {
          $global:path_advice += "`$env:WIN_DEBUG_LIBS='$WIN_DEBUG_LIBS'"
        }
      }
    }
  }

  if ((Test-Path "$vc_runtime_test_path" -PathType Leaf) -eq $false) {
    $global:install_errors += "$vc_runtime_test_path still not present, something went wrong."
  }

  if (-not (Found-Command($global:git))) {
    $global:install_errors += "not ok - git not installed."
  }

  if ($shbuild) {
    if (-not (Test-CommandVersion("cmake", $targetCmakeVersion))) {
      $global:install_errors += "not ok - unable to install cmake"
    } else {
      if ($cmakePath -ne '') {
        $global:path_advice += "`$env:PATH='$cmakePath;'+`$env:PATH"
        $env:PATH="$cmakePath\;$env:PATH"
      }
    }
  }

  if ($report_vc_vars_reqd) {
    if (Found-Command("clang++.exe")) {
      $file = (New-Object -ComObject Scripting.FileSystemObject).GetFile($(Get-CommandPath "clang++.exe"))
      $global:path_advice += "`$env:PATH='$($file.ParentFolder.Path)'+`$env:PATH"
    }
  }
}

Install-Requirements

if ($shbuild) {
  $gitPath = "$env:ProgramFiles\Git\bin"
  $sh = "$gitPath\sh.exe"

  if ($env:VERBOSE -eq "1") {
    Write-Log "v" "# Using shell $sh"
    iex "& ""$sh"" -c 'uname -s'"
  }

  # Look for sh in path
  if (-not (Found-Command($sh))) {
    $sh = "$gitPath\sh.exe"
    $global:install_errors += "not ok - sh.exe not in PATH or default Git\bin"
    Exit-IfErrors
  }

  $find_check = iex "& ""$sh"" -c 'find --version'" | Out-String

  if (-not ($find_check -like "*find (GNU findutils)*")) {
    $global:install_errors += "find is not GNU findutils: '$find_check'"
    Exit-IfErrors
  }

  $env:PATH="$BIN_PATH;$($env:PATH)"
  $global:path_advice += "`$env:PATH='$BIN_PATH;'+`$env:PATH"

  cd $OLD_CWD
  $install_sh = """$sh"" bin\install.sh $global:forceArg"
  Write-Log "h" "# Calling $install_sh"
  iex "& $install_sh"
  $exit=$LASTEXITCODE
  if ($exit -ne "0") {
    $global:install_errors += "$install_sh failed: $exit"
    Exit-IfErrors
  }
}

if ($global:path_advice.Count -gt 0) {
  Write-Log "h" "# Please run in future dev sessions: "
  Write-Log "h" "# (Or just run cd $OLD_CWD; .\bin\install.ps1)"
  foreach ($p in $global:path_advice) {
    Write-Log "h" $p
  }
}

if ($package_setup -eq $true) {
  $paths = @{}
  $file = (New-Object -ComObject Scripting.FileSystemObject).GetFile($(Get-CommandPath "clang++.exe"))
  $paths["CXX"] = $file.ShortPath
  ConvertTo-Json $paths > env.json
}

cd $OLD_CWD
Exit-IfErrors

Exit 0
