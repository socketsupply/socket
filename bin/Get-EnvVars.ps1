param($vars_arg)

(Get-ChildItem env:).GetEnumerator() | ForEach-Object{
  $output  = "{0}={1}" -f $_.key, $_.value
  Write-Output $output
}