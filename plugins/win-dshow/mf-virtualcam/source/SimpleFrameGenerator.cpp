// SPDX-License-Identifier: GPL-2.0-or-later
#include "pch.h"
#include <algorithm>
#include <cstring>

void SimpleFrameGenerator::disconnect()
{
	if (pipe != INVALID_HANDLE_VALUE)
		CloseHandle(pipe);
	pipe = INVALID_HANDLE_VALUE;
}

HRESULT SimpleFrameGenerator::Initialize(IMFMediaType *type)
{
	GUID subtype;
	UINT32 width, height;
	RETURN_IF_FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype));
	RETURN_IF_FAILED(MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height));
	RETURN_HR_IF(MF_E_INVALIDMEDIATYPE, subtype != MFVideoFormat_NV12 || width != OBS_MF_WIDTH ||
						height != OBS_MF_HEIGHT);
	RETURN_HR_IF(E_INVALIDARG, pipeName.rfind(OBS_MF_PIPE_PREFIX, 0) != 0 ||
				     pipeName.size() != wcslen(OBS_MF_PIPE_PREFIX) + 38);
	try { frame.resize(OBS_MF_FRAME_BYTES); }
	catch (...) { return E_OUTOFMEMORY; }
	return S_OK;
}

bool SimpleFrameGenerator::read_all(void *data, DWORD remaining)
{
	auto *cursor = static_cast<uint8_t *>(data);
	OVERLAPPED io{};
	io.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!io.hEvent)
		return false;
	const ULONGLONG deadline = GetTickCount64() + 250;
	bool ok = true;
	while (remaining && ok) {
		ResetEvent(io.hEvent);
		DWORD received = 0;
		ok = ReadFile(pipe, cursor, remaining, &received, &io) != FALSE;
		if (!ok && GetLastError() == ERROR_IO_PENDING) {
			ULONGLONG now = GetTickCount64();
			DWORD timeout = now < deadline ? static_cast<DWORD>(deadline - now) : 0;
			if (WaitForSingleObject(io.hEvent, timeout) != WAIT_OBJECT_0) {
				CancelIoEx(pipe, &io);
				GetOverlappedResult(pipe, &io, &received, TRUE);
				ok = false;
			} else {
				ok = GetOverlappedResult(pipe, &io, &received, FALSE) != FALSE;
			}
		}
		if (!ok || !received) {
			ok = false;
			break;
		}
		cursor += received;
		remaining -= received;
	}
	CloseHandle(io.hEvent);
	return ok;
}

HRESULT SimpleFrameGenerator::CreateFrame(BYTE *buffer, DWORD bytes, LONG pitch, ULONG)
{
	RETURN_HR_IF(E_INVALIDARG, !buffer || pitch < OBS_MF_WIDTH ||
				     uint64_t(pitch) * OBS_MF_HEIGHT * 3 / 2 > bytes);
	if (pipe == INVALID_HANDLE_VALUE)
		pipe = CreateFileW(pipeName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
				   FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
	obs_mf_frame_header header;
	bool received = pipe != INVALID_HANDLE_VALUE && read_all(&header, sizeof(header)) &&
			header.magic == OBS_MF_MAGIC && header.version == 1 && header.bytes == OBS_MF_FRAME_BYTES &&
			read_all(frame.data(), OBS_MF_FRAME_BYTES);
	if (!received) {
		disconnect();
		std::fill(frame.begin(), frame.begin() + OBS_MF_WIDTH * OBS_MF_HEIGHT, 16);
		std::fill(frame.begin() + OBS_MF_WIDTH * OBS_MF_HEIGHT, frame.end(), 128);
		Sleep(33);
	}
	for (size_t y = 0; y < OBS_MF_HEIGHT * 3 / 2; ++y) {
		std::memset(buffer + y * pitch, y < OBS_MF_HEIGHT ? 16 : 128, pitch);
		std::memcpy(buffer + y * pitch, frame.data() + y * OBS_MF_WIDTH, OBS_MF_WIDTH);
	}
	return S_OK;
}
