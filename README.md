# OBS MF Camera — experimental Windows 11 fork

An independent OBS Studio fork that adds a native Media Foundation software
camera alongside the DirectShow output. No custom kernel driver is included.
Targets Windows 11 x64, 1280×720 NV12 at approximately 30 fps.

**Experimental:** Windows Camera, Teams and Zoom compatibility still requires
manual validation. A successful build is not a guarantee of app compatibility.

## Windows installer

Open **Actions → Build OBS MF Windows installer → latest successful run** and
download **OBS-MF-Camera-Windows11-x64-Installer** from Artifacts. Extract the ZIP
and run the `Setup.exe` inside. If the run has failed or is still running, an
installer is not available yet. Build artifacts require a GitHub sign-in.

Setup installs the complete application into its own Program Files directory,
registers the MF media source and the fork's separate DirectShow camera, and
installs Microsoft's signed Visual C++ runtime. The experimental setup EXE
itself is unsigned. It requires administrator access and x64 Windows 11.

The installer build isolates application settings beneath
`%APPDATA%\obs-mf-camera\obs-studio`, uses a separate DirectShow CLSID and frame
queue, and disables the upstream updater. Import a scene collection if desired.
Uninstall it through Windows **Installed apps**.

## Use

1. Launch **OBS Studio MF (Experimental)** from the Start menu.
2. Prepare a scene, then click **Start Virtual Camera**.
3. Reopen the receiving camera app and select **OBS Studio MF (Windows Virtual Camera)**.
4. Check the OBS log for `Media Foundation camera started` if it is missing.

The device is session scoped and disappears when Virtual Camera stops. Windows
privacy controls apply. The fork's legacy camera is named **OBS MF DirectShow Camera**.

## Build from source

Follow upstream OBS Windows prerequisites, restore the NuGet dependencies in the
[backend guide](plugins/win-dshow/mf-virtualcam/README.md), then:

```powershell
cmake --preset windows-x64 -DOBS_MF_FORK_BUILD=ON "-DOBS_MF_NUGET_ROOT=$PWD/mf-deps"
cmake --build build_x64 --config Release --parallel
cmake --install build_x64 --config Release --component Runtime --prefix mf-app-stage
./plugins/win-dshow/mf-virtualcam/installer/Build-Installer.ps1 -StageDir mf-app-stage -OutputDir dist
```

The full fork currently uses Visual Studio 2026 and a Windows 11 SDK. Inno Setup
6.3+ is needed to create the installer. The standalone backend and transport tests
can also build independently with Visual Studio 2022 or newer.

Source is provided under OBS's GPL-2.0-or-later license. Retained Microsoft sample
code is MIT licensed; see its included license. This fork is not an official OBS
release. See [upstream OBS information](README.rst) and the
[technical implementation and acceptance checks](plugins/win-dshow/mf-virtualcam/README.md).
