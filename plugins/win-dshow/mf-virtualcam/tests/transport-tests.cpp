// SPDX-License-Identifier: GPL-2.0-or-later
#include "../transport.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>

static void check(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << " (Win32 " << GetLastError() << ")\n";
		std::exit(1);
	}
}

static bool read_all(HANDLE pipe, void *destination, DWORD remaining)
{
	auto *cursor = static_cast<uint8_t *>(destination);
	while (remaining) {
		DWORD received = 0;
		if (!ReadFile(pipe, cursor, remaining, &received, nullptr) || !received)
			return false;
		cursor += received;
		remaining -= received;
	}
	return true;
}

int main()
{
	// No connected client: destructor must cancel outstanding ConnectNamedPipe.
	auto begin = std::chrono::steady_clock::now();
	{
		obs_mf_transport idle;
		check(SUCCEEDED(idle.start()), "idle transport failed to start");
	}
	check(std::chrono::steady_clock::now() - begin < std::chrono::seconds(2), "idle shutdown hung");
	auto transport = std::make_unique<obs_mf_transport>();
	check(SUCCEEDED(transport->start()), "transport failed to start");
	std::vector<uint8_t> frame(OBS_MF_FRAME_BYTES, 128);
	std::fill(frame.begin(), frame.begin() + OBS_MF_WIDTH * OBS_MF_HEIGHT, 81);
	transport->push(frame.data());
	HANDLE first = CreateFileW(transport->pipe_name(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	HANDLE second = CreateFileW(transport->pipe_name(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	check(first != INVALID_HANDLE_VALUE && second != INVALID_HANDLE_VALUE, "clients could not connect");
	for (HANDLE client : {first, second}) {
		obs_mf_frame_header header;
		check(read_all(client, &header, sizeof(header)), "header read failed");
		check(header.magic == OBS_MF_MAGIC && header.version == 1 && header.bytes == frame.size(),
		      "invalid wire header");
		std::vector<uint8_t> received(frame.size());
		check(read_all(client, received.data(), static_cast<DWORD>(received.size())), "frame read failed");
		check(received == frame, "torn or incorrect frame");
	}
	// Clients remain connected but stop draining; cancellation must unblock WriteFile.
	Sleep(150);
	begin = std::chrono::steady_clock::now();
	transport.reset();
	check(std::chrono::steady_clock::now() - begin < std::chrono::seconds(2), "blocked-writer shutdown hung");
	CloseHandle(first);
	CloseHandle(second);
	std::cout << "Multi-client payload and cancellation checks passed\n";
}
