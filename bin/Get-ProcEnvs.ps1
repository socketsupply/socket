if ((Test-Path "bin" -PathType Container) -eq $false) {
  $get_env =".\Get-EnvVars.ps1"
} else {
  $get_env =".\bin\Get-EnvVars.ps1"
}

Function Get-ProcEnvs {
  param($command_string)

  $comspec = [Environment]::GetEnvironmentVariable("comspec")

  # run Get-EnvVars in same environment as command string so it can dump environment variables
  $command = "$comspec /k ""$command_string"" ""&"" powershell -File $get_env ""&"" exit"
  $output = iex "& $command" | Out-String

  $output -split "`r`n" | ForEach-Object {
    $data = $_.split("=")
    if ($data.Count -gt 1) {
      $old_val = [Environment]::GetEnvironmentVariable($data[0])
      if ($data[1].equals($old_val) -eq $false) { 
        [Environment]::SetEnvironmentVariable($data[0], $data[1])
      }
    }
  }

  $path = [Environment]::GetEnvironmentVariable("PATH")
}
