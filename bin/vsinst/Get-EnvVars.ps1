# dump environment variables so they can be imported into parent shell
# Has to be a separate script because it has to run in subshell after another process sets vars
param($vars_arg)

(Get-ChildItem env:).GetEnumerator() | ForEach-Object{
  $output  = "{0}={1}" -f $_.key, $_.value
  Write-Output $output
}