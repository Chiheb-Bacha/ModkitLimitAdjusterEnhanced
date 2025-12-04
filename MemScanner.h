#pragma once
#include <cstdint>
#include <string>

namespace MemScanner
{
    uint8_t* FindPatternBmh(const std::string& pattern);
    uint8_t* FindPatternBmh(const std::string& pattern, uint8_t* startAddress);
    uint8_t* FindPatternBmh(const std::string& pattern, uint8_t* startAddress, uint64_t size);
}
