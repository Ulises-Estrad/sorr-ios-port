$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolBin = Join-Path $root "portable-tools\w64devkit\bin"
$depBin = Join-Path $root "portable-deps\msys2-mingw32\mingw32\bin"
$dataDir = Join-Path $root "sorr-vita-master\data"
$exe = Join-Path $dataDir "..\..\build-portable\bgdi.exe"

$env:PATH = "$toolBin;$depBin;$env:PATH"

Push-Location $dataDir
try {
    & $exe SorR.dat
}
finally {
    Pop-Location
}
