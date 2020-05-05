
$tempFile = "$env:temp\vcvars.txt"

if ($env:CI_JOB_NAME -eq "build:windows_vs2019") {
  cmd.exe /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat`" && set > $tempFile"
} else {
  cmd.exe /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat`" && set > $tempFile"
}

Get-Content "$tempFile" | Foreach-Object {
  if ($_ -match "^(.*?)=(.*)$") {
    Set-Content "env:\$($matches[1])" $matches[2]
  }
}
