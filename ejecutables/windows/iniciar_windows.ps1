$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LogDir = Join-Path $Root "logs"
$Log = Join-Path $LogDir "lanzador-windows.log"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
Add-Content $Log "===== $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') Windows ====="

try {
    $CMake = (Get-Command cmake -ErrorAction Stop).Source
    $Build = Join-Path $Root "build-windows"
    & $CMake -S $Root -B $Build -G "Visual Studio 17 2022" -A x64 *>> $Log
    if ($LASTEXITCODE -ne 0) { throw "Falló la configuración CMake." }
    & $CMake --build $Build --config Release --parallel 4 *>> $Log
    if ($LASTEXITCODE -ne 0) { throw "Falló la compilación Release." }
    $Output = Join-Path $Build "Release"
    & $CMake -E copy_directory (Join-Path $Root "assets") (Join-Path $Output "assets") *>> $Log
    & $CMake -E copy_directory (Join-Path $Root "shaders") (Join-Path $Output "shaders") *>> $Log
    Start-Process -FilePath (Join-Path $Output "TexturizadoCempasuchil.exe") -WorkingDirectory $Output
} catch {
    Add-Content $Log "ERROR: $($_.Exception.Message)"
    exit 1
}
