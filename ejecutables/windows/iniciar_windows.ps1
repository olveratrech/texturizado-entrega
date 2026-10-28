$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$LogDir = Join-Path $Root "logs"
$Log = Join-Path $LogDir "lanzador-windows.log"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
Add-Content $Log "===== $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') Windows ====="

try {
    $CMake = (Get-Command cmake -ErrorAction Stop).Source

    # Elegir el generador que corresponde a la instancia instalada. Visual Studio
    # 2026 usa la versión 18; mantener compatibilidad con equipos que aún usan 2022.
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $VsWhere)) {
        $VsWhere = Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe"
    }
    if (-not (Test-Path $VsWhere)) { throw "No se encontró Visual Studio Installer (vswhere.exe)." }
    $VersionVisualStudio = (& $VsWhere -latest -products * -property installationVersion | Select-Object -First 1)
    if (-not $VersionVisualStudio) { throw "No se encontró una instalación de Visual Studio." }
    $VersionMayor = [int]($VersionVisualStudio.Split('.')[0])
    if ($VersionMayor -ge 18) {
        $Generador = "Visual Studio 18 2026"
        $EtiquetaGenerador = "vs18"
    } elseif ($VersionMayor -eq 17) {
        $Generador = "Visual Studio 17 2022"
        $EtiquetaGenerador = "vs17"
    } else {
        throw "La versión de Visual Studio $VersionVisualStudio no está soportada."
    }
    Add-Content $Log "Visual Studio $VersionVisualStudio; generador: $Generador"
    $Build = Join-Path $Root "build-windows-$EtiquetaGenerador"

    # Windows PowerShell convierte cada línea de stderr de un programa nativo en
    # ErrorRecord. Con ErrorActionPreference=Stop eso interrumpía CMake tras la
    # primera línea y ocultaba el diagnóstico restante. Esta función conserva la
    # salida completa y devuelve el código real del proceso.
    function Invoke-CMakeLogged([string[]] $Arguments) {
        $preferenciaAnterior = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        try {
            & $CMake @Arguments 2>&1 | ForEach-Object {
                Add-Content -Path $Log -Value $_.ToString()
            }
            return $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $preferenciaAnterior
        }
    }

    $codigo = Invoke-CMakeLogged @("-S", $Root, "-B", $Build, "-G", $Generador, "-A", "x64")
    if ($codigo -ne 0) { throw "Falló la configuración CMake." }
    $codigo = Invoke-CMakeLogged @("--build", $Build, "--config", "Release", "--parallel", "4")
    if ($codigo -ne 0) { throw "Falló la compilación Release." }
    $Output = Join-Path $Build "Release"
    $codigo = Invoke-CMakeLogged @("-E", "copy_directory", (Join-Path $Root "assets"), (Join-Path $Output "assets"))
    if ($codigo -ne 0) { throw "No se pudieron copiar los assets." }
    $codigo = Invoke-CMakeLogged @("-E", "copy_directory", (Join-Path $Root "shaders"), (Join-Path $Output "shaders"))
    if ($codigo -ne 0) { throw "No se pudieron copiar los shaders." }
    Start-Process -FilePath (Join-Path $Output "TexturizadoCempasuchil.exe") -WorkingDirectory $Output
} catch {
    Add-Content $Log "ERROR: $($_.Exception.Message)"
    exit 1
}
