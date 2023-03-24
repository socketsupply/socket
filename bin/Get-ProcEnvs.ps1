Function Get-ProcEnvs () {
  param($params)
  $path = $params[0]
  $command_string = $params[1]
  
  $get_env ="$path\bin\Get-EnvVars.ps1"
  if ((Test-Path -Path $get_env) -eq $false) {
    $get_env ="$path\Get-EnvVars.ps1"
  }

  # PS on WSL path note: Microsoft.PowerShell.Core\FileSystem::\\wsl.localhost\distro won't work
  # The given path's format is not supported, doesn't work with replace below.
  # $path = $path.replace("Microsoft.PowerShell.Core\FileSystem::", "")
  # NOTE: This doesn't fix other issues, eg WSL folder is read only
  if ($get_env -like "*Microsoft.PowerShell.Core\FileSystem::*") {
    $temp_env="$env:TEMP\get-env-$(New-Guid).ps1"
    Copy-Item "$get_env" -Dest $temp_env
    $get_env = $temp_env
  }

  $comspec = [Environment]::GetEnvironmentVariable("comspec")
  $debug = ($null -ne [Environment]::GetEnvironmentVariable("debug"))

  # run Get-EnvVars in same environment as command string so it can dump environment variables
  $command = "$comspec /k ""$command_string"" ""&"" powershell -File $get_env ""&"" exit"
  $output = iex "& $command" -ErrorAction SilentlyContinue -ErrorVariable errors | Out-String
  Write-Output "$LASTEXITCODE"


  if ($debug) {
    Write-Host "Get-ProcEnvs: debug $debug"
    Write-Output "$command_string output:"
  }

  if ($temp_env -ne $null) {
    Remove-Item $temp_env
  }

  # this doesn't catch anything
  if ($errors -ne $null) {
    foreach($error in $errors) {
      Write-Output "Error: $error"
    }
  }

  $output -split "`r`n" | ForEach-Object {

    $data = $_.split("=")
    if ($data.Count -gt 1) {
      $old_val = [Environment]::GetEnvironmentVariable($data[0])
      if ($data[1].equals($old_val) -eq $false) {
        if ($debug) {
          Write-Output "set $($data[0])=$($data[1]))"
        }
        [Environment]::SetEnvironmentVariable($data[0], $data[1])
      }
    } elseif ($debug) { # Only write new vars or non env var lines
      Write-Output "$_"
    }
  }

  $path = [Environment]::GetEnvironmentVariable("PATH")
}
