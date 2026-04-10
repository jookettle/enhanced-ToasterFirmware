$ids = Get-Process -Name node | Select-Object -ExpandProperty Id
foreach ($id in $ids) {
  $p = Get-CimInstance Win32_Process -Filter "ProcessId=$id"
  Write-Output "PID=$($p.ProcessId)"
  Write-Output $p.CommandLine
  Write-Output "----"
}
