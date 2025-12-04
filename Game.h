#pragma once

#include <string>
#include <vector>

struct PatternPair
{
	std::string pattern;
	int offset;
};

struct PatternPairExtended
{
	std::string pattern;
	int offset;
	int mulModifier;
	int addModifier;
};

void initializeGame();
void RelocateAbsolueModkitArray(void* newModkitArray, const std::vector<PatternPair>& list);
void RelocateRelativeModkitArray(void* newModkitArray, const std::vector<PatternPair>& list);
void AdjustModkitArrayLimit();
LPVOID FindPrevFreeRegion(LPVOID pAddress, LPVOID pMinAddr, DWORD dwAllocationGranularity);
void* AllocateStubMemory(size_t size);
void FatalErrorExit();
void patchHardcodedArrayLimits();