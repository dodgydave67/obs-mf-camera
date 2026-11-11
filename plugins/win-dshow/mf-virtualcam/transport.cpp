// SPDX-License-Identifier: GPL-2.0-or-later
#include "transport.hpp"
#include <sddl.h>
#include <objbase.h>
#include <algorithm>
#include <cstring>

obs_mf_transport::~obs_mf_transport()
{
	if (stop_event)
		SetEvent(stop_event);
	for (auto &thread : threads)
		if (thread.joinable())
			thread.join();
	for (HANDLE pipe : pipes)
		if (pipe && pipe != INVALID_HANDLE_VALUE)
			CloseHandle(pipe);
	if (stop_event)
		CloseHandle(stop_event);
}

HRESULT obs_mf_transport::start()
{
	GUID id;
	HRESULT hr = CoCreateGuid(&id);
	if (FAILED(hr))
		return hr;
	wchar_t text[40];
	StringFromGUID2(id, text, 40);
	name = std::wstring(OBS_MF_PIPE_PREFIX) + text;
	frame.resize(OBS_MF_FRAME_BYTES);
	std::fill(frame.begin(), frame.begin() + OBS_MF_WIDTH * OBS_MF_HEIGHT, 16);
	std::fill(frame.begin() + OBS_MF_WIDTH * OBS_MF_HEIGHT, frame.end(), 128);
	stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!stop_event)
		return HRESULT_FROM_WIN32(GetLastError());
	PSECURITY_DESCRIPTOR descriptor = nullptr;
	if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
		    L"D:P(A;;GA;;;SY)(A;;GA;;;OW)(A;;GR;;;LS)", SDDL_REVISION_1, &descriptor, nullptr))
		return HRESULT_FROM_WIN32(GetLastError());
	SECURITY_ATTRIBUTES security = {sizeof(security), descriptor, FALSE};
	for (size_t i = 0; i < pipes.size(); ++i) {
		pipes[i] = CreateNamedPipeW(name.c_str(),
					   PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED |
						   (i == 0 ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
					   PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
					   static_cast<DWORD>(pipes.size()), 64 * 1024, 0, 0, &security);
		if (pipes[i] == INVALID_HANDLE_VALUE) {
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}
	}
	LocalFree(descriptor);
	if (FAILED(hr))
		return hr;
	for (size_t i = 0; i < pipes.size(); ++i)
		threads[i] = std::thread([this, i] { serve(pipes[i]); });
	return S_OK;
}

bool obs_mf_transport::await_io(HANDLE pipe, OVERLAPPED &io, DWORD &bytes)
{
	HANDLE events[] = {stop_event, io.hEvent};
	if (WaitForMultipleObjects(2, events, FALSE, INFINITE) != WAIT_OBJECT_0 + 1) {
		CancelIoEx(pipe, &io);
		// The OVERLAPPED and buffer must outlive cancellation completion.
		GetOverlappedResult(pipe, &io, &bytes, TRUE);
		return false;
	}
	return GetOverlappedResult(pipe, &io, &bytes, FALSE) != FALSE;
}

bool obs_mf_transport::write_all(HANDLE pipe, const void *data, DWORD remaining)
{
	const auto *cursor = static_cast<const uint8_t *>(data);
	OVERLAPPED io{};
	io.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!io.hEvent)
		return false;
	bool ok = true;
	while (remaining && ok) {
		ResetEvent(io.hEvent);
		DWORD written = 0;
		ok = WriteFile(pipe, cursor, remaining, &written, &io) != FALSE;
		if (!ok && GetLastError() == ERROR_IO_PENDING)
			ok = await_io(pipe, io, written);
		if (!ok || !written) {
			ok = false;
			break;
		}
		cursor += written;
		remaining -= written;
	}
	CloseHandle(io.hEvent);
	return ok;
}

void obs_mf_transport::serve(HANDLE pipe) noexcept
{
	try {
		std::vector<uint8_t> snapshot(OBS_MF_FRAME_BYTES);
		while (WaitForSingleObject(stop_event, 0) == WAIT_TIMEOUT) {
			OVERLAPPED io{};
			io.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
			if (!io.hEvent)
				return;
			bool connected = ConnectNamedPipe(pipe, &io) != FALSE;
			DWORD error = connected ? ERROR_SUCCESS : GetLastError();
			DWORD ignored = 0;
			if (error == ERROR_IO_PENDING)
				connected = await_io(pipe, io, ignored);
			else if (error == ERROR_PIPE_CONNECTED)
				connected = true;
			CloseHandle(io.hEvent);
			obs_mf_frame_header header;
			while (connected && WaitForSingleObject(stop_event, 33) == WAIT_TIMEOUT) {
				{
					std::lock_guard<std::mutex> lock(frame_mutex);
					std::memcpy(snapshot.data(), frame.data(), frame.size());
				}
				++header.sequence;
				connected = write_all(pipe, &header, sizeof(header)) &&
					    write_all(pipe, snapshot.data(), static_cast<DWORD>(snapshot.size()));
			}
			DisconnectNamedPipe(pipe);
		}
	} catch (...) {
		// Allocation failure closes this producer; consumer times out to black.
		DisconnectNamedPipe(pipe);
	}
}

void obs_mf_transport::push(const uint8_t *nv12)
{
	std::lock_guard<std::mutex> lock(frame_mutex);
	std::memcpy(frame.data(), nv12, frame.size());
}
