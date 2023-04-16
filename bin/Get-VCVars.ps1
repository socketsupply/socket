Function Get-VCVars {
  $vs_releases = @('2022')

  # Locate VC Vars outside of %ProgramFiles(x86)%
  $shortcut_path_template="$env:ProgramData\Microsoft\Windows\Start Menu\Programs\Visual Studio %RELEASE\Visual Studio Tools\VC\x64 Native Tools Command Prompt for VS %RELEASE.lnk"

  foreach ($r in $vs_releases) {
    $shortcut_path = $shortcut_path_template.replace("%RELEASE", $r)
    if (Test-Path $shortcut_path -PathType leaf) {
      $WScript = New-Object -ComObject WScript.Shell
      $shortcut = $WScript.CreateShortcut($shortcut_path)
      $test_path = $shortcut.Arguments.replace("/k ", "").replace("""", "")
      if (Test-Path $test_path -PathType leaf) {
        return $true, $test_path
      }
    }
  }

  # Fall back to previous method
  $pf = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
  $mvs = "Microsoft Visual Studio"
  $suffix = "VC\Auxiliary\Build\vcvars64.bat"

  $vs_editions = @('Enterprise', 'Professional', 'Community', 'BuildTools')

  foreach ($r in $vs_releases) {
    foreach ($e in $vs_editions) {
      $test_path = "$pf\$mvs\$r\$e\$suffix"
      if (Test-Path $test_path -PathType leaf) {
        return $true, $test_path
      }
    }
  }

  return $false
}