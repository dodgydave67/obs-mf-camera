// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

class SimpleFrameGenerator {
	HANDLE pipe = INVALID_HANDLE_VALUE;
	std::wstring pipeName;
	std::vector<uint8_t> frame;
	bool read_all(void *data, DWORD bytes);
	void disconnect();

public:
	~SimpleFrameGenerator() { disconnect(); }
	void SetPipeName(const std::wstring &name) { pipeName = name; }
	HRESULT Initialize(IMFMediaType *type);
	HRESULT CreateFrame(BYTE *buffer, DWORD bytes, LONG pitch, ULONG unused);
};
