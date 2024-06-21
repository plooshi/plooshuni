#pragma once
#include "pch.h"
#include "SDK.h"

uint64_t FindWorldNetMode() {
	if (floor(FNVer) == 18) return Memcury::Scanner::FindPattern("48 83 EC 28 48 83 79 ? ? 75 20 48 8B 91 ? ? ? ? 48 85 D2 74 1E 48 8B 02 48 8B CA FF 90").Get();

	auto stringRef = Memcury::Scanner::FindStringRef(L"PREPHYSBONES");
	auto fBegin = stringRef.ScanFor({ 0x40, 0x55 }, false).Get();

	for (int i = 0; i < 0x400; i++) {
		// check for 0xe8
		if (*(uint8_t*)(fBegin + i) == 0xe8 && *(uint8_t*)(fBegin + i - 1) != 0x8b) {
			return Memcury::Scanner(fBegin + i).RelativeOffset(1).Get();
		}
	}
	
	return 0;
}

uint64 FindGIsClient()
{
	auto Addr = Memcury::Scanner::FindStringRef(L"AllowCommandletRendering");

	std::vector<std::vector<uint8_t>> BytesToCheck = {
		{0x88, 0x05},
		{0xC6, 0x05},
		{0x88, 0x1D},
		{0x44, 0x88}
	};
	int Picked = 0;
	int Skip = 2;

	for (int i = 0; i < 50; i++)
	{
		auto Curr = (uint8_t*)(Addr.Get() - i);
		for (auto& Bytes : BytesToCheck)
		{
			if (*Curr == Bytes[0]) {
				bool Found = true;
				for (int j = 1; j < Bytes.size(); j++)
				{
					if (*(Curr + j) != Bytes[j])
					{
						Found = false;
						break;
					}
				}
				if (Found) {
					auto Relative = Bytes[0] == 0x44 ? 3 : 2;

					if (Bytes[0] == 0x44 && *(Curr + 2) == 0x74) continue;

					if (!Picked) Picked = Bytes[0];
					else if (Picked != Bytes[0]) continue;

					if (Skip > 0) {
						Skip--;
						continue;
					}

					auto Scanner = Memcury::Scanner(Curr);
					return Bytes[0] == 0xC6 ? Scanner.RelativeOffset(Relative, 1).Get() : Scanner.RelativeOffset(Relative).Get();
				}
			}
		}
	}
	return 0;
}

uint64 FindTickFlush() {
	if (FNVer >= 16.40) {
		auto addr = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 B8 0F 29 78 A8 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 8A", false).Get();
		if (!addr) addr = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 B8 0F 29 78 A8 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 0F", false).Get();
		if (!addr) addr = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 B8 0F 29 78 A8 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B F9 48 89 4D 38 48 8D 4D 40").Get();

		return addr;
	}
	auto Addr = Memcury::Scanner::FindStringRef(L"STAT_NetTickFlush", false);

	if (!Addr.Get())
	{
		if (FNVer >= 2.5 && FNVer <= 4.5) return Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 45 0F 29 43 ? 45 0F 29 4B ? 48 8B 05 ? ? ? ? 48 33").Get();
	}

	return Addr.ScanFor(FNVer < 18 ? std::vector<uint8_t> { 0x4C, 0x8B } : std::vector<uint8_t> { 0x48, 0x8B, 0xC4 }, false, 0, 0, 1000).Get();
}

uint64 FindInitHost()
{
	if (FNVer >= 16.40)
	{
		auto addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B F1 4C 8D 05", false).Get();
		if (!addr) addr = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 56 41 57 48 8D 68 A1 48 81 EC ? ? ? ? 48 8B F1 4C 8D 35 ? ? ? ? 4D").Get();
        if (FNVer == 18.10) addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B F1 48 8D 1D ? ? ? ? 4C 8B C3 48 8D 4D D7 45").Get();

		return addr;
	}

	auto Addr = Memcury::Scanner::FindStringRef(L"BeaconPort=");
	return Addr.ScanFor({ 0x48, 0x8B, 0xC4 }, false, 0, 0, 1000).Get();
}

uint64 FindPauseBeaconRequests() {
	if (FNVer >= 19.00) return Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 ED 48 8B F1 84 D2 74 27 80 3D").Get();
	else if (FNVer >= 16.40) return Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 33 F6 48 8B F9 84 D2 74").Get();
	else if (FNVer >= 13) {
		auto Addr = Memcury::Scanner::FindPattern("40 57 48 83 EC 30 48 8B F9 84 D2 74 62 80 3D").Get();
		if (!Addr) Addr = Memcury::Scanner::FindPattern("40 53 48 83 EC 30 48 8B ? 84 D2 74 ? 80 3D").Get();
		return Addr;
	}
	else if (FNVer >= 2.5 && FNVer <= 4.5) return Memcury::Scanner::FindPattern("40 53 48 83 EC 30 48 8B D9 84 D2 74 68 80 3D ? ? ? ? ? 72 2C 48 8B 05 ? ? ? ? 4C 8D 44").Get();
	else if (FNVer == 6.30 || FNVer == 6.31) return Memcury::Scanner::FindPattern("40 53 48 83 EC 30 48 8B D9 84 D2 74 68 80 3D").Get();
	else if (FNVer == 0) {
		auto Addr = Memcury::Scanner::FindPattern("40 53 48 83 EC 30 48 8B D9 84 D2 74 6F 80 3D ? ? ? ? ? 72 33 48 8B 05").Get();
		if (!Addr) Addr = Memcury::Scanner::FindPattern("40 53 48 83 EC 30 48 8B D9 84 D2 74 6F 80 3D", false).Get();
		if (!Addr) Addr = Memcury::Scanner::FindPattern("40 53 48 83 EC 30 48 8B D9 84 D2 74 68 80 3D ? ? ? ? ? 72").Get();

		return Addr;
	}

	auto Addr = Memcury::Scanner::FindStringRef(L"All Beacon Requests Resumed.");
	return Addr.ScanFor({ 0x40, 0x53 }, false, 0, 1, 1000).Get();
}

uint64 FindInitListen()
{
	if (FNVer >= 19.00) return Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 10 49 89 73 18 57 48 83 EC 50 48 8B BC 24 ?? ?? ?? ?? 49 8B F0 48 8B").Get();

	if (FNVer >= 16.40) return Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 73 10 57 48 83 EC 50 48 8B BC 24 ? ? ? ? 49 8B F0 48 8B 01 48 8B").Get();

	auto Addr = Memcury::Scanner::FindStringRef(L"%s IpNetDriver listening on port %i");
	return Addr.ScanFor(std::vector<uint8_t>{ 0x48, 0x89, 0x5C }, false, 1, 0, 2000).Get();
}

uint64 FindSetWorld()
{
	if (FNVer < 13.00) return Memcury::Scanner::FindStringRef(L"AOnlineBeaconHost::InitHost failed").ScanFor({ 0x48, 0x8B, 0xD0, 0xE8 }, false).RelativeOffset(4).Get();

	int VTIndex = 0;

	int Season = (int) std::floor(FNVer);
	if (Season == 13) VTIndex = 0x70;
	else if (Season == 14 || FNVer <= 15.2) VTIndex = 0x71;
	else if (FNVer >= 15.3 && Season < 18) VTIndex = 0x72;
	else if (Season == 18) VTIndex = 0x73;
	else if (Season >= 19 && Season < 21) VTIndex = 0x7A;
	if (Season == 20) VTIndex = 0x7B;
	if (Season >= 21) VTIndex = 0x7C;

	return __int64(GetDefaultObj<"NetDriver">()->Vft[VTIndex]);
}

std::vector<uint64_t> NullFuncs = {};
std::vector<uint64_t> RetTrueFuncs = {};

void ProcessNullsAndRetTrues() {
	DWORD og;
	for (auto& Func : NullFuncs) {
		if (Func == 0x0) continue;
		VirtualProtect((void*)Func, 1, PAGE_EXECUTE_READWRITE, &og);
		*(uint8*)Func = 0xC3;
		VirtualProtect((void*)Func, 1, og, &og);
	}

	auto RetTruePoint = Memcury::Scanner::FindPattern("B8 01 00 00 00 C3").Get();
	for (auto& Func : RetTrueFuncs) {
		if (Func == 0x0) continue;
		VirtualProtect((void*)Func, 5, PAGE_EXECUTE_READWRITE, &og);
		auto AddrForJmp = RetTruePoint - Func + 5;
		*(uint8*)Func = 0xE9;
		*(uint32*)(Func + 1) = uint32(AddrForJmp);
		VirtualProtect((void*)Func, 5, og, &og);
	}
}

uint64 FindGetMaxTickRate()
{
	if (FNVer >= 19.00) return Memcury::Scanner::FindPattern("40 53 48 83 EC 50 0F 29 74 24 ? 48 8B D9 0F 29 7C 24 ? 0F 28 F9 44 0F 29").Get();

	if (FNVer >= 16.40) return Memcury::Scanner::FindPattern("40 53 48 83 EC 60 0F 29 74 24 ? 48 8B D9 0F 29 7C 24 ? 0F 28").Get();

	auto sRef = Memcury::Scanner::FindStringRef(L"Hitching by request!").Get();

	if (!sRef)
		return 0;

	for (int i = 0; i < 400; i++)
	{
		if (*(uint8_t*)(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(uint8_t*)(sRef - i + 1) == 0x53)
		{
			return sRef - i;
		}

		if (*(uint8_t*)(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(uint8_t*)(sRef - i + 2) == 0x5C)
		{
			return sRef - i;
		}
	}

	return 0;
}

uint64 FindDispatchRequest()
{
	auto sRef = Memcury::Scanner::FindStringRef(L"MCP-Profile: Dispatching request to %s", false, 0, FNVer >= 19).Get();

	if (!sRef) return 0;

	for (int i = 0; i < 1000; i++)
	{
		if (*(uint8_t*)(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(uint8_t*)(sRef - i + 2) == 0x5C)
		{
			return sRef - i;
		}

		if (*(uint8_t*)(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(uint8_t*)(sRef - i + 2) == 0xC4)
		{
			return sRef - i;
		}
	}

	return 0;
}