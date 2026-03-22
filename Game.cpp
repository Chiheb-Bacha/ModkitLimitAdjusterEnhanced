#include "pch.h"
#include <cstdint>
#include <string_view>
#include <cassert>
#include "Game.h"
#include <fstream>
#include "Logging.h"

bool g_isEnhanced;
uint32_t VanillaModkitArray_Size;
uint32_t ModkitArray_Size;
void* newArray;

void initializeGame()
{
	ClearLog();

	ModkitArray_Size = (uint32_t)GetPrivateProfileIntA("MODKIT_SETTINGS", "MODKITARRAY_SIZE", 65536, ".\\ModkitLimitAdjusterEnhanced.ini");
	g_isEnhanced = [] {
		char path[MAX_PATH];
		GetModuleFileNameA(GetModuleHandleA(nullptr), path, MAX_PATH);

		const char* filename = strrchr(path, '\\');
		filename = filename ? filename + 1 : path;

		return (_stricmp(filename, "GTA5_Enhanced.exe") == 0);
	}();

	newArray = (void*)AllocateStubMemory(sizeof(uint16_t) * ModkitArray_Size);

	if (!g_isEnhanced)
	{
		RelocateAbsolueModkitArray(newArray, {
			{ "73 ? 8b c2 0f b7 84 56", 8 },
			{ "0f b7 43 ? 0f b7 84 46", 8 }
		});
	}

	AdjustModkitArrayLimit();
	patchHardcodedArrayLimits();

	std::string msg = "Vanilla Modkit List Size is: " + std::to_string(VanillaModkitArray_Size) + "\n"
		+ "Modkit List Size Adjusted to: " + std::to_string(ModkitArray_Size);

	Logging(msg);
}

void OverrideModkitArraySize(uint32_t newSize, const std::vector<PatternPairExtended>& list) {

	for (auto& entry : list) {
		auto address = MemScanner::FindPatternBmh(entry.pattern);
		address += entry.offset;
		DWORD oldProtect;
		VirtualProtect(address, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &oldProtect);
		if (VanillaModkitArray_Size == 0) {
			VanillaModkitArray_Size = (uint32_t)((*(uint32_t*)address - entry.addModifier) / entry.mulModifier);
		}
		*(uint32_t*)address = (uint32_t)((newSize * entry.mulModifier) + entry.addModifier);
		VirtualProtect(address, sizeof(uint32_t), oldProtect, &oldProtect);
	}
}

void RelocateAbsolueModkitArray(void* newModkitArray, const std::vector<PatternPair>& list) {

	int32_t oldAddress = 0;

	uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
	int32_t newArrayRVA = static_cast<int32_t>(reinterpret_cast<uintptr_t>(newModkitArray) - moduleBase);

	for (auto& entry : list)
	{
		auto address = MemScanner::FindPatternBmh(entry.pattern);
		address += entry.offset;
		if (!oldAddress)
		{
			oldAddress = *(uint32_t*)address;
		}

		auto curTarget = *(uint32_t*)address;
		assert(curTarget == oldAddress);

		DWORD oldProtect;
		VirtualProtect(address, sizeof(int32_t), PAGE_EXECUTE_READWRITE, &oldProtect);
		*(int32_t*)address = newArrayRVA;
		VirtualProtect(address, sizeof(int32_t), oldProtect, &oldProtect);
	}

}

void RelocateRelativeModkitArray(void* newModkitArray, const std::vector<PatternPair>& list)
{
	void* oldAddress = nullptr;

	for (auto& entry : list)
	{
		auto address = MemScanner::FindPatternBmh(entry.pattern);
		address += entry.offset;
		if (!oldAddress)
		{
			oldAddress = *(int32_t*)address + address + 4;
		}

		auto curTarget = *(int32_t*)address + address + 4;;
		assert(curTarget == oldAddress);

		DWORD oldProtect;
		VirtualProtect(address, sizeof(int32_t), PAGE_EXECUTE_READWRITE, &oldProtect);
		*(int32_t*)address = static_cast<int32_t>((intptr_t)newModkitArray - (intptr_t)address - 4);
		VirtualProtect(address, sizeof(int32_t), oldProtect, &oldProtect);
	}
}

void AdjustModkitArrayLimit()
{
	std::vector<PatternPair> patternPairs;

	if (g_isEnhanced) {
		patternPairs = {
			// Data
			{ "31 c9 48 8d 05 ? ? ? ? eb", 5 },
			{ "89 c2 48 8d 0d ? ? ? ? eb", 5 },
			{ "41 0f b7 c2 48 8d 0d ? ? ? ? 0f b7 04 41", 7 },
			{ "57 48 83 ec ? 48 8d 0d ? ? ? ? 41 b8", 8 },
			{ "48 8d 3d ? ? ? ? 0f b7 3c 47", 3 },
			{ "77 ? 48 8d 15 ? ? ? ? 0f b7 14 42", 5 }
		};
	}
	else {
		patternPairs = {
			// Data
			{ "7d ? 41 bc ? ? ? ? 4c 8d 3d", 11 },
			{ "45 33 c0 4c 8d 0d ? ? ? ? b9", 6 },
			{ "48 8d 0d ? ? ? ? 0f b7 c0", 3 },
			{ "48 8d 0d ? ? ? ? 0f b7 c6", 3 },
			{ "48 8d 3d ? ? ? ? b9 ? ? ? ? 0f b7 c0", 3 }
		};
	}

	RelocateRelativeModkitArray(newArray, patternPairs);
}

void patchHardcodedArrayLimits() {

	std::vector<PatternPairExtended> patternPairs;

	if (g_isEnhanced) {
		patternPairs = {
			// Data
			{ "0f b7 c0 66 ba ? ? 3d ? ? ? ? 77", 8, 1, -1 },
			{ "48 83 c5 ? 48 83 c0 ? 48 83 c7", 15, 1, 3 },
			{ "74 ? 48 83 c5 ? 48 83 c3 ? 48 83 c0 ? 48 3d", 16, 1, 3 },
			{ "48 83 c7 ? 48 83 c2 ? 48 83 c5 ? 48 81 fd", 15, 1, 3 },
			{ "48 83 c7 ? 48 83 c2 ? 48 83 c6 ? 48 81 fe", 15, 1, 3 },
			{ "41 b8 ? ? ? ? b2 ? e8 ? ? ? ? 66 0f 76 c0", 2, 2, 0 },
			{ "0f b7 41 ? 66 bf ? ? 48 3d", 10, 1, -1 },
			{ "41 0f b7 ca 66 b8 ? ? 81 f9", 10, 1, -1 }
		};
	}
	else {
		patternPairs = {
			// Data
			{ "b8 ? ? ? ? 66 3b f0 73", 1, 1, 0 },
			{ "41 b8 ? ? ? ? 66 41 3b c0 73 ? 48 8d 0d", 2, 1, 0 },
			{ "b9 ? ? ? ? 66 3b d1 73 ? 8b c2", 1, 1, 0 },
			{ "b9 ? ? ? ? 66 39 4b ? 73", 1, 1, 0 },
			{ "41 81 f8 ? ? ? ? 7c ? 0f b7 c1", 3, 1, 0 },
			{ "b9 ? ? ? ? 0f b7 c0 b2", 1, 1, 0 }
		};
	}

	OverrideModkitArraySize(ModkitArray_Size, patternPairs);
}

LPVOID FindPrevFreeRegion(LPVOID pAddress, LPVOID pMinAddr, DWORD dwAllocationGranularity)
{
	ULONG_PTR tryAddr = (ULONG_PTR)pAddress;

	// Round down to the next allocation granularity.
	tryAddr -= tryAddr % dwAllocationGranularity;

	// Start from the previous allocation granularity multiply.
	tryAddr -= dwAllocationGranularity;

	while (tryAddr >= (ULONG_PTR)pMinAddr)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery((LPVOID)tryAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) ==
			0)
			break;

		if (mbi.State == MEM_FREE)
			return (LPVOID)tryAddr;

		if ((ULONG_PTR)mbi.AllocationBase < dwAllocationGranularity)
			break;

		tryAddr = (ULONG_PTR)mbi.AllocationBase - dwAllocationGranularity;
	}

	return NULL;
}

void* AllocateStubMemory(size_t size)
{
	// Max range for seeking a memory block. (= 1024MB)
	const uint64_t MAX_MEMORY_RANGE = 0x40000000;

	void* origin = GetModuleHandle(NULL);

	ULONG_PTR minAddr;
	ULONG_PTR maxAddr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	minAddr = (ULONG_PTR)si.lpMinimumApplicationAddress;
	maxAddr = (ULONG_PTR)si.lpMaximumApplicationAddress;

	if ((ULONG_PTR)origin > MAX_MEMORY_RANGE &&
		minAddr < (ULONG_PTR)origin - MAX_MEMORY_RANGE)
		minAddr = (ULONG_PTR)origin - MAX_MEMORY_RANGE;

	if (maxAddr > (ULONG_PTR)origin + MAX_MEMORY_RANGE)
		maxAddr = (ULONG_PTR)origin + MAX_MEMORY_RANGE;

	LPVOID pAlloc = origin;

	void* stub = nullptr;
	while ((ULONG_PTR)pAlloc >= minAddr)
	{
		pAlloc = FindPrevFreeRegion(pAlloc, (LPVOID)minAddr, si.dwAllocationGranularity);
		if (pAlloc == NULL)
			break;

		stub = VirtualAlloc(pAlloc, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (stub != NULL)
			break;
	}

	return stub;
}

void FatalErrorExit() {
	Logging("FATAL ERROR, EXITING...");

	MessageBoxA(
		nullptr,
		"FATAL ERROR.\nCheck the Log for details.",
		"ModkitLimitAdjuster Enhanced",
		MB_OK | MB_ICONERROR
	);

	exit(1);
}