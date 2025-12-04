//
// Copyright (C) 2015 crosire & kagikn & contributors
// Copyright (C) 2025 Chiheb-Bacha
// License: https://github.com/scripthookvdotnet/scripthookvdotnet#license
// Source: https://github.com/Chiheb-Bacha/ScriptHookVDotNetEnhanced/blob/main/source/core/MemScanner.cs
//

#include "pch.h"
#include "MemScanner.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <windows.h>
#include <psapi.h>
#include "Logging.h"
#include <algorithm>
#include <array>

namespace MemScanner
{
    struct ModuleInfo
    {
        uint8_t* base = nullptr;
        uint64_t size = 0;

        ModuleInfo()
        {
            HMODULE mod = GetModuleHandleA(nullptr);
            MODULEINFO info{};
            GetModuleInformation(GetCurrentProcess(), mod, &info, sizeof(info));
            base = reinterpret_cast<uint8_t*>(info.lpBaseOfDll);
            size = static_cast<uint64_t>(info.SizeOfImage);
        }
    };

    static ModuleInfo& GetModuleInfo()
    {
        static ModuleInfo cache;
        return cache;
    }

    static std::array<short, 256> CreateShiftTableForBmh(const std::vector<short>& pattern)
    {
        std::array<short, 256> skipTable;
        int lastIndex = (int)pattern.size() - 1;
        int lastWildcardIndex = -1;
        for (int i = (int)pattern.size() - 1; i >= 0; --i)
        {
            if (pattern[i] < 0)
            {
                lastWildcardIndex = i;
                break;
            }
        }

        int diff = lastIndex - max(lastWildcardIndex, 0);
        if (diff == 0) diff = 1;

        skipTable.fill((short)diff);

        for (int i = lastIndex - diff; i < lastIndex; i++)
        {
            short val = pattern[i];
            if (val >= 0)
                skipTable[val] = (short)(lastIndex - i);
        }

        return skipTable;
    }

    static void ParsePattern(const std::string& pat, std::vector<uint8_t>& outPattern, std::string& outMask)
    {
        outPattern.clear();
        outMask.clear();

        std::istringstream ss(pat);
        std::string token;

        while (ss >> token)
        {
            if (token == "?" || token == "??")
            {
                outPattern.push_back(0);
                outMask.push_back('?');
            }
            else
            {
                uint8_t val = (uint8_t)std::stoi(token, nullptr, 16);
                outPattern.push_back(val);
                outMask.push_back('x');
            }
        }
    }

    uint8_t* FindPatternBmh(const std::vector<uint8_t>& pattern, const std::string& mask, uint8_t* startAddress, uint64_t size)
    {
        if (!startAddress || size == 0 || pattern.empty() || pattern.size() != mask.size())
            return nullptr;

        std::vector<short> patternArray(pattern.size());
        for (size_t i = 0; i < pattern.size(); i++)
        {
            patternArray[i] = (mask[i] != '?') ? (short)pattern[i] : (short)-1;
        }

        int lastIndex = (int)patternArray.size() - 1;
        auto skipTable = CreateShiftTableForBmh(patternArray);

        uint8_t* endAddr = startAddress + size - patternArray.size();

        for (uint8_t* cur = startAddress; cur <= endAddr;)
        {
            int i = lastIndex;
            while (i >= 0 && (patternArray[i] < 0 || cur[i] == patternArray[i]))
                --i;

            if (i < 0)
                return cur;

            int shift = skipTable[cur[lastIndex]];
            if (shift <= 0) shift = 1;
            cur += shift;
        }


        return nullptr;
    }

    uint8_t* FindPatternBmh(const std::string& patternStr, std::string& maskStr, uint8_t* startAddress, uint64_t size)
    {
        std::vector<uint8_t> pattern;
        std::string mask;
        ParsePattern(patternStr, pattern, mask);
        maskStr = mask;

        auto address = FindPatternBmh(pattern, mask, startAddress, size);

        if (!address) {
            Logging("Pattern not found: " + patternStr);
            FatalErrorExit();
        }

        return address;
    }

    uint8_t* FindPatternBmh(const std::string& patternStr, uint8_t* startAddress, uint64_t size)
    {
        std::string mask;
        return FindPatternBmh(patternStr, mask, startAddress, size);
    }

    uint8_t* FindPatternBmh(const std::string& patternStr)
    {
        auto& module = GetModuleInfo();
        return FindPatternBmh(patternStr, module.base, module.size);
    }

    uint8_t* FindPatternBmh(const std::string& patternStr, uint8_t* startAddress)
    {
        auto& module = GetModuleInfo();
        if (startAddress < module.base) return nullptr;
        uint64_t size = module.size - (uint64_t)(startAddress - module.base);
        return FindPatternBmh(patternStr, startAddress, size);
    }
}
