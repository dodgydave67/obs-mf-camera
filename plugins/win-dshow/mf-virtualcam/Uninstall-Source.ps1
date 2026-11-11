#Requires -RunAsAdministrator
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
if (-not [Environment]::Is64BitProcess) { throw 'Run 64-bit PowerShell.' }
$class = 'HKLM:\SOFTWARE\Classes\CLSID\{BD34B55A-51CA-4A96-BEB4-601C35D572A9}'
if (Test-Path -LiteralPath $class) { Remove-Item -LiteralPath $class -Recurse -Force }
$dll = Join-Path $env:ProgramFiles 'OBS-MFVirtualCamera\obs-mf-camera-source.dll'
if (Test-Path -LiteralPath $dll) { Remove-Item -LiteralPath $dll -Force }
Write-Output 'Media source removed. Session cameras disappear when OBS stops Virtual Camera. If the DLL is in use, close camera apps and reboot before deleting it.'
