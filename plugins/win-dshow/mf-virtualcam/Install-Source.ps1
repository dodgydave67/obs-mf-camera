#Requires -RunAsAdministrator
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$SourceDll)
$ErrorActionPreference = 'Stop'
if (-not [Environment]::Is64BitProcess) { throw 'Run 64-bit PowerShell.' }
if ([Environment]::OSVersion.Version.Build -lt 22000) { throw 'Windows 11 is required.' }
$source = (Resolve-Path -LiteralPath $SourceDll).Path
if ([IO.Path]::GetFileName($source) -ne 'obs-mf-camera-source.dll') { throw 'Select obs-mf-camera-source.dll.' }
$destination = Join-Path $env:ProgramFiles 'OBS-MFVirtualCamera'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
$installed = Join-Path $destination 'obs-mf-camera-source.dll'
Copy-Item -LiteralPath $source -Destination $installed -Force
$class = 'HKLM:\SOFTWARE\Classes\CLSID\{BD34B55A-51CA-4A96-BEB4-601C35D572A9}'
New-Item -Path "$class\InprocServer32" -Force | Out-Null
Set-Item -Path $class -Value 'OBS Media Foundation Camera Source'
Set-Item -Path "$class\InprocServer32" -Value $installed
New-ItemProperty -Path "$class\InprocServer32" -Name ThreadingModel -Value Both -PropertyType String -Force | Out-Null
Write-Output 'Media source installed. Start Virtual Camera in the modified OBS build, then reopen your camera app.'
