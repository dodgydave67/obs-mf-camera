// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "protocol.h"

// One producer; up to four independent Frame Server source instances. Pipe ACLs
// allow only the owner, SYSTEM, and Local Service. No global writable mapping.
class obs_mf_transport {
	HANDLE stop_event = nullptr;
	std::array<HANDLE, 4> pipes{};
	std::array<std::thread, 4> threads;
	std::mutex frame_mutex;
	std::vector<uint8_t> frame;
	std::wstring name;
	void serve(HANDLE pipe) noexcept;
	bool await_io(HANDLE pipe, OVERLAPPED &io, DWORD &bytes);
	bool write_all(HANDLE pipe, const void *data, DWORD bytes);

public:
	obs_mf_transport() = default;
	obs_mf_transport(const obs_mf_transport &) = delete;
	obs_mf_transport &operator=(const obs_mf_transport &) = delete;
	~obs_mf_transport();
	HRESULT start();
	void push(const uint8_t *nv12);
	const wchar_t *pipe_name() const { return name.c_str(); }
};
