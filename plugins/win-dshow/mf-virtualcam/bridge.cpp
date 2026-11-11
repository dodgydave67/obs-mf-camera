// SPDX-License-Identifier: GPL-2.0-or-later
#include "bridge.h"
#include "transport.hpp"
#include "scale.hpp"
#include <mfapi.h>
#include <mfvirtualcamera.h>
#include <wrl/client.h>
#include <memory>
#include <new>

using Microsoft::WRL::ComPtr;

struct obs_mf_camera {
	obs_mf_transport transport;
	HANDLE stop = nullptr;
	HANDLE ready = nullptr;
	std::thread worker;
	HRESULT result = E_FAIL;
	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> scaled;

	~obs_mf_camera()
	{
		if (stop)
			SetEvent(stop);
		if (worker.joinable())
			worker.join();
		if (stop)
			CloseHandle(stop);
		if (ready)
			CloseHandle(ready);
	}

	void run() noexcept
	{
		HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		HRESULT mf = FAILED(com) ? com : MFStartup(MF_VERSION, MFSTARTUP_LITE);
		HMODULE library = nullptr;
		ComPtr<IMFVirtualCamera> camera;
		result = mf;
		if (SUCCEEDED(result)) {
			library = LoadLibraryExW(L"mfsensorgroup.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			result = library ? S_OK : HRESULT_FROM_WIN32(GetLastError());
		}
		if (SUCCEEDED(result)) {
			auto create = reinterpret_cast<decltype(&MFCreateVirtualCamera)>(
				GetProcAddress(library, "MFCreateVirtualCamera"));
			result = create ? create(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session,
						MFVirtualCameraAccess_CurrentUser, L"OBS Studio MF", OBS_MF_SOURCE_CLSID,
						nullptr, 0, &camera)
					: HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
		}
		if (SUCCEEDED(result))
			result = camera->SetString(OBS_MF_PIPE_NAME, transport.pipe_name());
		if (SUCCEEDED(result))
			result = camera->Start(nullptr);
		SetEvent(ready);
		if (SUCCEEDED(result))
			WaitForSingleObject(stop, INFINITE);
		if (camera)
			camera->Shutdown();
		camera.Reset();
		if (library)
			FreeLibrary(library);
		if (SUCCEEDED(mf))
			MFShutdown();
		if (SUCCEEDED(com))
			CoUninitialize();
	}
};

extern "C" obs_mf_camera *obs_mf_camera_start(uint32_t width, uint32_t height, long *error)
{
	if (!error)
		return nullptr;
	*error = E_INVALIDARG;
	if (!width || !height || (width & 1) || (height & 1))
		return nullptr;
	try {
		auto camera = std::make_unique<obs_mf_camera>();
		camera->width = width;
		camera->height = height;
		camera->scaled.resize(OBS_MF_FRAME_BYTES);
		camera->stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		camera->ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!camera->stop || !camera->ready) {
			*error = HRESULT_FROM_WIN32(GetLastError());
			return nullptr;
		}
		*error = camera->transport.start();
		if (FAILED(*error))
			return nullptr;
		camera->worker = std::thread([ptr = camera.get()] { ptr->run(); });
		WaitForSingleObject(camera->ready, INFINITE);
		*error = camera->result;
		return SUCCEEDED(*error) ? camera.release() : nullptr;
	} catch (...) {
		*error = E_OUTOFMEMORY;
		return nullptr;
	}
}

extern "C" void obs_mf_camera_frame(obs_mf_camera *camera, uint8_t **planes, uint32_t *strides)
{
	if (!camera || !planes || !strides || !planes[0] || !planes[1] || strides[0] < camera->width ||
	    strides[1] < camera->width)
		return;
	obs_mf_scale_nv12(camera->scaled.data(), OBS_MF_WIDTH, OBS_MF_HEIGHT, planes[0], planes[1], camera->width,
			  camera->height, strides[0], strides[1]);
	camera->transport.push(camera->scaled.data());
}

extern "C" void obs_mf_camera_stop(obs_mf_camera *camera)
{
	delete camera;
}
