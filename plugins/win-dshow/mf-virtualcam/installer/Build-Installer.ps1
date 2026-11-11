[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$StageDir,
    [string]$OutputDir = 'dist',
    [ValidatePattern('^\d+\.\d+\.\d+$')][string]$InstallerVersion = '0.1.0',
    [ValidatePattern('^[a-zA-Z0-9.-]+$')][string]$BuildRevision = 'local'
)
$ErrorActionPreference = 'Stop'
$stage = (Resolve-Path -LiteralPath $StageDir).Path
$required = @(
    'bin/64bit/obs64.exe',
    'core/win-dshow/win-dshow.dll',
    'core/win-dshow/data/obs-virtualcam-module32.dll',
    'core/win-dshow/data/obs-virtualcam-module64.dll',
    'mf-camera/obs-mf-camera-source.dll'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $stage $relative))) {
        throw "Required runtime file is missing: $relative"
    }
}
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
Copy-Item -LiteralPath (Join-Path $repoRoot 'COPYING') -Destination (Join-Path $stage 'COPYING') -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'experimental-notice.txt') -Destination (Join-Path $stage 'INSTALL-MF-CAMERA.txt') -Force

$prerequisites = Join-Path $stage 'prerequisites'
New-Item -ItemType Directory -Force -Path $prerequisites | Out-Null
$runtime = Join-Path $prerequisites 'VC_redist.x64.exe'
Invoke-WebRequest -Uri 'https://aka.ms/vc14/vc_redist.x64.exe' -OutFile $runtime
$signature = Get-AuthenticodeSignature -LiteralPath $runtime
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') {
    throw 'The downloaded Visual C++ runtime does not have a valid Microsoft signature.'
}

$compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source
if (-not $compiler) {
    foreach ($version in @('7', '6')) {
        $candidate = Join-Path ${env:ProgramFiles(x86)} "Inno Setup $version/ISCC.exe"
        if (Test-Path -LiteralPath $candidate) { $compiler = $candidate; break }
    }
}
if (-not $compiler) { throw 'Install Inno Setup 6.3 or newer, then run this script again.' }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDir).Path
& $compiler "/DStageDir=$stage" "/DOutputDir=$output" "/DInstallerVersion=$InstallerVersion" "/DBuildRevision=$BuildRevision" (Join-Path $PSScriptRoot 'obs-mf-camera.iss')
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed with exit code $LASTEXITCODE" }
$installer = Join-Path $output "OBS-MF-Camera-$InstallerVersion-$BuildRevision-Windows11-x64-Setup.exe"
if (-not (Test-Path -LiteralPath $installer)) { throw 'Inno Setup did not produce the expected installer.' }
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $installer).Hash.ToLowerInvariant()
"$hash  $([IO.Path]::GetFileName($installer))" | Set-Content -Encoding ascii -LiteralPath "$installer.sha256"
Write-Output $installer
