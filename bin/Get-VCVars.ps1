Function Get-VCVars {
  $pf = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
  $mvs = "Microsoft Visual Studio"
  $suffix = "VC\Auxiliary\Build\vcvars64.bat"

  $vs_releases = @('2022')
  $vs_editions = @('Enterprise', 'Professional', 'Community', 'BuildTools')

  Foreach ($r in $vs_releases) {
    Foreach ($e in $vs_editions) {
      $test_path = "$pf\$mvs\$r\$e\$suffix"
      if (Test-Path $test_path -PathType leaf) {
        return $true, $test_path
      }
    }
  }

  return $false
}