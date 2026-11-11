# Experimental OBS Media Foundation camera

This is an **experimental implementation**, not a validated camera compatibility release.
It adds a Media Foundation software camera to a fork of OBS Studio so Windows 11
camera clients can discover OBS without relying on the existing DirectShow filter.
No custom kernel driver is involved. Windows 10 and ARM64 are outside this backend's
current scope. Compatibility with every app is not guaranteed.

The backend is opt-in with `ENABLE_MF_VIRTUALCAM=ON`. Start Virtual Camera publishes
an additional camera called **OBS Studio MF (Windows Virtual Camera)**; Windows
controls the final display name. The existing DirectShow camera remains available.
Stop Virtual Camera removes the session-scoped MF camera. Camera applications may
need reopening after starting it.

## What is implemented

- An `IMFActivate` / `IMFMediaSourceEx` / `IMFMediaStream2` COM DLL based on Microsoft's
  MIT-licensed VirtualCamera sample, with a separate CLSID.
- A Windows 11 `MFCreateVirtualCamera` controller on an MTA worker thread, using
  session lifetime and current-user access. The API is loaded dynamically so the
  modified OBS plugin can still load on Windows 10 and use DirectShow.
- Direct OBS NV12 frame delivery over random named pipes across the user-session /
  Frame Server boundary, without opening the DirectShow device or needing global
  shared-memory creation privileges.
- Restricted pipe ACLs for the owner, SYSTEM and Local Service; remote connections
  rejected; fixed-size, versioned packets; bounded consumer reads; cancellation of
  pending server I/O on shutdown. Four pipe instances accommodate independent
  Frame Server activations. Actual app concurrency needs Windows validation.
- Fixed 1280 x 720 NV12 at approximately 30 fps. Other OBS output sizes are stretched
  using nearest-neighbor scaling. Frame selection is latest-frame based; this first
  version does not preserve OBS frame timestamps, aspect ratio or HDR metadata.
- Stride-aware scaling checks and Windows transport tests for payload integrity,
  concurrent readers, idle shutdown and cancellation of blocked writes.
- Standalone Windows build workflow, plus manual source install / removal scripts.

## Validation status

| Check | Status |
|---|---|
| Portable NV12 scaling tests with AddressSanitizer and UndefinedBehaviorSanitizer | Passed on Linux; leak detection disabled because the host uses ptrace |
| Windows MSVC standalone build and transport tests | Passed on GitHub's Windows runner |
| Full OBS build and installer | Build workflow added; inspect its latest result |
| COM registration and activation under Frame Server's Local Service identity | Not run |
| Windows Camera, Settings, Teams, Zoom, Discord and browsers | Not run |
| Install, stop/start, uninstall, sleep/resume, privacy denial and multi-user isolation | Not run |

The full Windows build and real-device checks are release blockers. Do not describe
this prototype as a proven fix or distribute its DLL as a production release.
Starting/stopping the MF controller currently waits for its worker, so native
camera-service delays can delay OBS's start/stop operation. Production work should
make this asynchronous and surface backend status in the OBS UI.

## Build the camera source and tests on Windows

For the standalone backend, install Visual Studio 2022 or newer with Desktop development with C++, a Windows 11 SDK
(10.0.22000.0 or later), CMake 3.28+ and NuGet CLI. Use x64 Windows 11 for camera
testing. Keep the Microsoft Visual C++ runtime installed (the OBS installer normally
provides it). Run the following from the repository root in PowerShell:

```powershell
nuget install Microsoft.Windows.ImplementationLibrary -Version 1.0.260126.7 -OutputDirectory mf-deps -NonInteractive
nuget install Microsoft.Windows.CppWinRT -Version 3.0.260520.1 -OutputDirectory mf-deps -NonInteractive
$deps = (Resolve-Path mf-deps).Path
cmake -S plugins/win-dshow/mf-virtualcam -B build-mf -A x64 "-DOBS_MF_NUGET_ROOT=$deps" -DBUILD_TESTING=ON
cmake --build build-mf --config Release --parallel
ctest --test-dir build-mf -C Release --output-on-failure
cmake --install build-mf --config Release --prefix mf-stage
```

The `Build OBS MF Windows installer` GitHub Actions workflow first performs this
standalone build and runs transport and scaling tests on a Windows runner, then
builds the full OBS application and an Inno Setup installer. Hosted Windows Server
build success is not evidence of Windows 11 camera compatibility.

## Build the modified OBS application

Follow OBS's [Windows build instructions](https://github.com/obsproject/obs-studio/wiki/Build-Instructions-For-Windows),
including submodules, OBS dependencies and the normal DirectShow camera setup.
The checked-out OBS master preset currently names Visual Studio 18 2026; use its
required toolchain and a CMake version that supports that generator for the full build.
Then configure the x64 preset with these additional options:

```powershell
git fetch --tags https://github.com/obsproject/obs-studio.git
cmake --preset windows-x64 -DENABLE_MF_VIRTUALCAM=ON "-DOBS_MF_NUGET_ROOT=$deps"
cmake --build build_x64 --config RelWithDebInfo --parallel
```

If your build environment uses a different preset/configuration, keep its normal
settings and add the two `-D` options above. This backend-only mode requires the
separate COM setup described below. For a full isolated application installer,
use `-DOBS_MF_FORK_BUILD=ON`, stage the Runtime component and run
`installer/Build-Installer.ps1` as shown in the repository's main README.
That mode uses separate settings, queue and DirectShow identity and disables
the upstream updater. The installer handles COM registration automatically.

## Install the experimental media source

Use a test machine. Close OBS and all camera clients before installation or update.
In **64-bit PowerShell as Administrator**, run:

```powershell
& .\plugins\win-dshow\mf-virtualcam\Install-Source.ps1 -SourceDll .\build-mf\Release\obs-mf-camera-source.dll
```

The script copies only this DLL to `C:\Program Files\OBS-MFVirtualCamera` (using the
actual Program Files path on your system), then registers its x64 COM CLSID in HKLM.
The fixed Program Files location allows the service to read the DLL. Do not register
it from Downloads, a user profile, a network share, or a folder writable by ordinary
users. No kernel driver or Frame Server registry workaround is installed.

Launch the **modified OBS build** as your normal user, allow camera access in Windows
privacy settings, and click Start Virtual Camera. Inspect the OBS log for
`Media Foundation camera started`. A failure logs an HRESULT and leaves DirectShow
output running; DirectShow fallback does not fix MF-only applications. Select the
additional OBS Studio MF camera in the receiving app.

To remove it, stop Virtual Camera, close OBS and camera clients, and run:

```powershell
& .\plugins\win-dshow\mf-virtualcam\Uninstall-Source.ps1
```

Windows may retain the DLL in a camera-service process until that process exits.
If removal reports the file is in use, reboot and run removal again. Do not remove
the original OBS DirectShow registration for this backend.

## Required Windows acceptance checks

1. With camera access allowed, confirm the new device enumerates in Windows Camera
   and Settings while the modified OBS is running Virtual Camera. Record OS build.
2. Test a moving scene, text, color bars, a source switch and a non-720p OBS output.
   Confirm correct UV order, row pitch, timestamps and no stale frame after stop.
3. Test Camera, the current Teams client, Zoom, Discord and two browsers separately
   and concurrently. Record client versions; do not infer support from enumeration.
4. Repeatedly start/stop the output; exit/restart OBS; disconnect clients mid-frame;
   confirm clean removal and that the original DirectShow camera still works.
5. Deny camera access, remove COM registration, remove the source DLL and test on
   Windows 10. Confirm failure reporting and that DirectShow can still operate.
6. Test sleep/resume and a second logged-in Windows user. Verify that cameras remain
   user scoped and other users cannot read the producer's named pipe.
7. Validate signed distribution, runtime dependencies, installation rollback and
   uninstall. Integrate the source into the main installer before shipping a release.

## Provenance and references

- OBS base: `obsproject/obs-studio`, commit `fe1ed7955f300efd17fc59f65938388e43d8bb06`.
- Media source adapted from `microsoft/Windows-Camera`, commit
  `626f8b19c5f367602f2e89c6b314573d3776c9df`, `Samples/VirtualCamera`.
  The retained Microsoft code is MIT licensed; see `source/LICENSE.Microsoft`.
  New code is GPL-2.0-or-later, consistent with OBS. Distribute OBS's COPYING too.
- [Microsoft MFCreateVirtualCamera API](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera)
- [Microsoft VirtualCamera sample](https://github.com/microsoft/Windows-Camera/tree/master/Samples/VirtualCamera)
- [OBS issue discussing the DirectShow / Media Foundation limitation](https://github.com/obsproject/obs-studio/issues/13439)
