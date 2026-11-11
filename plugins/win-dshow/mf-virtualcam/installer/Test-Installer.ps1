# Runs only on disposable GitHub Actions hosts, never on a user's workstation.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$Installer)
$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'This smoke test is limited to disposable GitHub Actions runners.' }
$setup = (Resolve-Path -LiteralPath $Installer).Path
$appRoot = Join-Path $env:ProgramFiles 'OBS Studio MF (Experimental)'
if (Test-Path -LiteralPath $appRoot) { throw 'Refusing to modify an existing application installation.' }
$log = Join-Path (Split-Path -Parent $setup) 'installer-smoke.log'
$installed = Start-Process -FilePath $setup -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/LOG=`"$log`"") -Wait -PassThru
if ($installed.ExitCode -notin @(0, 3010)) { throw "Setup failed with code $($installed.ExitCode). See $log" }
try {
    foreach ($relative in @('bin/64bit/obs64.exe', 'mf-camera/obs-mf-camera-source.dll', 'unins000.exe')) {
        if (-not (Test-Path -LiteralPath (Join-Path $appRoot $relative))) { throw "Missing installed file: $relative" }
    }
    foreach ($view in @([Microsoft.Win32.RegistryView]::Registry64, [Microsoft.Win32.RegistryView]::Registry32)) {
        $registry = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine, $view)
        try {
            $camera = $registry.OpenSubKey('Software\Classes\CLSID\{703018E1-1834-48E9-8896-5EC54AA23476}\InprocServer32')
            if (-not $camera) { throw "DirectShow registration missing from $view" }
            try {
                if (-not ([string]$camera.GetValue('')).StartsWith($appRoot, [StringComparison]::OrdinalIgnoreCase)) {
                    throw "DirectShow camera points outside the installed fork ($view)"
                }
            } finally { $camera.Dispose() }
        } finally { $registry.Dispose() }
    }
    $source = Get-Item -LiteralPath 'HKLM:\SOFTWARE\Classes\CLSID\{BD34B55A-51CA-4A96-BEB4-601C35D572A9}\InprocServer32'
    if ($source.GetValue('') -ne (Join-Path $appRoot 'mf-camera\obs-mf-camera-source.dll')) {
        throw 'Media Foundation registration points at the wrong source DLL.'
    }
} finally {
    $uninstaller = Join-Path $appRoot 'unins000.exe'
    if (Test-Path -LiteralPath $uninstaller) {
        $removed = Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -Wait -PassThru
        if ($removed.ExitCode -notin @(0, 3010)) { throw "Uninstall failed with code $($removed.ExitCode)" }
    }
}
if (Test-Path -LiteralPath (Join-Path $appRoot 'bin/64bit/obs64.exe')) { throw 'Uninstaller left the main application binary installed.' }
if (Test-Path -LiteralPath 'HKLM:\SOFTWARE\Classes\CLSID\{BD34B55A-51CA-4A96-BEB4-601C35D572A9}') {
    throw 'Uninstaller left the Media Foundation source registered.'
}
Write-Output 'Silent install, camera registration, and uninstall smoke checks passed.'
