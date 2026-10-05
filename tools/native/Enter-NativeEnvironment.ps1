param(
    [switch]$RequireQt,
    [switch]$RequirePython,
    [string]$QtRoot = 'C:\Qt\6.11.2\msvc2022_64',
    [string]$ReceiptPath
)

$ErrorActionPreference = 'Stop'
# vcvarsall appends paths. Restore this process's original inputs before each
# invocation so dot-sourcing repeatedly cannot grow cmd's environment past 8K.
if (-not (Get-Variable -Name acNativeInitialEnvironment -Scope Script -ErrorAction SilentlyContinue)) {
    $script:acNativeInitialEnvironment = @{}
    foreach ($acKey in @('PATH','INCLUDE','LIB','LIBPATH','VSCMD_VER','__VSCMD_PREINIT_PATH',
                        'VCToolsVersion','VCToolsInstallDir','VCToolsRedistDir','VCINSTALLDIR')) {
        $script:acNativeInitialEnvironment[$acKey] = [Environment]::GetEnvironmentVariable($acKey, 'Process')
    }
}
foreach ($acKey in $script:acNativeInitialEnvironment.Keys) {
    [Environment]::SetEnvironmentVariable($acKey, $script:acNativeInitialEnvironment[$acKey], 'Process')
}
# Prefer English when the language resource is installed. CMake also detects
# localized /showIncludes output in Ninja's actual encoding.
$env:VSLANG = '1033'
$acVswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$acComponent = 'Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64'
$acVsPaths = @(& $acVswhere -all -products '*' -requires $acComponent -property installationPath)
if ($LASTEXITCODE -ne 0 -or $acVsPaths.Count -ne 1) { throw 'A unique v143 Visual Studio instance is required.' }
$acVsPath = $acVsPaths[0].Trim()
$acVcvars = Join-Path $acVsPath 'VC\Auxiliary\Build\vcvarsall.bat'
if (-not (Test-Path -LiteralPath $acVcvars)) { throw 'vcvarsall.bat was not found.' }

# Import the selected toolset into this process; setting variables inside a
# detached child shell would not initialize the subsequent CMake invocation.
$acCommand = 'call "' + $acVcvars + '" x64 -vcvars_ver=14.44 >nul && set'
$acEnvironment = & $env:ComSpec /d /s /c $acCommand
if ($LASTEXITCODE -ne 0) { throw 'v143 x64 environment initialization failed.' }
foreach ($acLine in $acEnvironment) {
    if ($acLine -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}
$acNativeCMake = Join-Path $acVsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$acNativeNinja = Join-Path $acVsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
$acNativePython = Join-Path $env:LOCALAPPDATA 'Programs\Python\Python314'
if (-not (Test-Path -LiteralPath (Join-Path $acNativePython 'python.exe'))) {
    $acPythonCommand = Get-Command python.exe -All -ErrorAction SilentlyContinue |
        Where-Object { $_.Source -notmatch '(?i)msys|mingw|ucrt|WindowsApps' } | Select-Object -First 1
    $acNativePython = if ($acPythonCommand) { Split-Path -Parent $acPythonCommand.Source } else { $null }
}
if ($RequirePython -and -not $acNativePython) { throw 'Native Python is required for tests or development tools.' }
$acQt = $QtRoot
foreach ($acExecutable in @((Join-Path $acNativeCMake 'cmake.exe'), (Join-Path $acNativeNinja 'ninja.exe'))) {
    if (-not (Test-Path -LiteralPath $acExecutable)) { throw ('Native tool is missing: ' + $acExecutable) }
}
$acRetainedPaths = @($env:PATH -split ';' | Where-Object { $_ -and $_ -notmatch '(?i)\\(?:msys64|mingw64|ucrt64)(?:\\|$)' })
$env:PATH = (@($acNativeCMake, $acNativeNinja, $acNativePython, (Join-Path $acQt 'bin')) + $acRetainedPaths | Select-Object -Unique) -join ';'
$env:CMAKE_PREFIX_PATH = $acQt
$env:Qt6_DIR = Join-Path $acQt 'lib\cmake\Qt6'
$env:QTDIR = $acQt
$acCompiler = (Get-Command cl.exe -ErrorAction Stop).Source
$acRedist = @(Get-ChildItem -LiteralPath (Join-Path $acVsPath 'VC\Redist\MSVC') -Directory |
    Where-Object { $_.Name -match '^14\.44\.' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'x64\Microsoft.VC143.CRT\vcruntime140.dll')) })
if ($acRedist.Count -ne 1) { throw 'A unique v143 14.44 Release REDIST source is required.' }
$env:VCToolsRedistDir = $acRedist[0].FullName + '\'
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VCToolsVersion -notmatch '^14\.44\.' -or $acCompiler -notmatch '\\14\.44\.[^\\]+\\bin\\Hostx64\\x64\\cl\.exe$') {
    throw 'The selected compiler is not v143 14.44 Hostx64/x64.'
}
if ($RequireQt -and -not (Test-Path -LiteralPath (Join-Path $env:Qt6_DIR 'Qt6Config.cmake'))) {
    throw 'The official Qt 6.11.2 MSVC 2022 x64 kit is missing.'
}
if ($RequireQt) {
    foreach ($acQtFile in @('lib\cmake\Qt6ShaderTools\Qt6ShaderToolsConfig.cmake', 'bin\qsb.exe', 'bin\Qt6Core.dll', 'bin\Qt6Cored.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $acQt $acQtFile))) {
            throw ('The required Qt kit component is missing: ' + $acQtFile)
        }
    }
}
$acEnvironmentReceipt = [pscustomobject]@{
    toolset = $env:VCToolsVersion
    targetArchitecture = $env:VSCMD_ARG_TGT_ARCH
    compiler = $acCompiler
    linker = (Get-Command link.exe).Source
    cmake = (Get-Command cmake.exe).Source
    ninja = (Get-Command ninja.exe).Source
    python = if ($acNativePython) { Join-Path $acNativePython 'python.exe' } else { $null }
    sdk = $env:WindowsSdkDir
    sdkVersion = $env:WindowsSDKVersion
    redist = $env:VCToolsRedistDir
    qtPrefix = $acQt
    qtPresent = (Test-Path -LiteralPath (Join-Path $env:Qt6_DIR 'Qt6Config.cmake'))
    initializedAt = [DateTimeOffset]::UtcNow.ToString('o')
}
if ($ReceiptPath) { $acEnvironmentReceipt | ConvertTo-Json | Set-Content -LiteralPath $ReceiptPath -Encoding utf8 }
$acEnvironmentReceipt
