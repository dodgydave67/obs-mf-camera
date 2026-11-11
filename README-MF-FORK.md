# OBS Windows 11 Media Foundation camera fork

This branch contains an experimental Media Foundation virtual camera backend.
It is an unvalidated source prototype, not a working Windows binary release.

See [build, install and validation instructions](plugins/win-dshow/mf-virtualcam/README.md).

The changes are opt-in: build OBS x64 with `ENABLE_MF_VIRTUALCAM=ON` and a restored
`OBS_MF_NUGET_ROOT`, install the separate COM source, and start Virtual Camera.
The original DirectShow backend remains active. The new camera targets Windows 11,
720p NV12 at approximately 30 fps. It uses Microsoft's software-camera API and does
not require a custom kernel driver.
