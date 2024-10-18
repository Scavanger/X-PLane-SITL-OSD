
#pragma once

#include "platform.h"

#include <string>
#include <sstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <XPLMUtilities.h>
#include "XPLMPlugin.h"
#include <string.h>

using namespace std::chrono;

const std::string CONFIG_FILE_NAME = "inavSitlOsd.ini";
const std::string FONTS_DIR_NAME   = "fonts";

namespace Helper {
    template<class... Args>
    inline void Log(Args... args) 
    {
        std::stringstream strStream;
        strStream << "INAV SITL OSD: ";
        (strStream << ... << args);
        strStream << std::endl;
        XPLMDebugString(strStream.str().c_str());
    }

    inline std::filesystem::path getPluginDir()
    {
        char path[MAX_PATH];
        XPLMGetPluginInfo(XPLMGetMyID(), NULL, path, NULL, NULL);
        return std::filesystem::path(path).parent_path();        
    }

    inline std::vector<std::filesystem::path> getFontPaths(std::string subPath, bool directories)  
    {
        std::vector<std::filesystem::path> fontList;
        std::filesystem::path path = getPluginDir().append(FONTS_DIR_NAME).append(subPath);
        if (std::filesystem::exists(path)) {
            for (auto dirEntry = std::filesystem::recursive_directory_iterator(path); dirEntry != std::filesystem::recursive_directory_iterator(); ++dirEntry) {
            dirEntry.disable_recursion_pending();
            if (dirEntry->is_directory() && directories)
                fontList.push_back(dirEntry->path());
            if (dirEntry->is_regular_file() && !directories)
                fontList.push_back(dirEntry->path());
            }
        }
        return fontList;
    }

    inline std::filesystem::path getConfigFileName() 
    {
        char prefPath[MAX_PATH]; 
        XPLMGetPrefsPath(prefPath);
        XPLMExtractFileAndPath(prefPath);
        return std::filesystem::path(prefPath).append(CONFIG_FILE_NAME);;        
    }

    inline uint8_t getLowerByte(uint16_t value) {
        return static_cast<uint8_t>(value & 0xFF);
    }

    inline uint8_t getUpperByte(uint16_t value) {
        return static_cast<uint8_t>((value & 0xFF00) >> 8);
    }
#ifdef LINUX
    inline uint32_t getTickCount()
    {
        struct timespec spec;
        clock_gettime(CLOCK_MONOTONIC, &spec);
        return static_cast<uint32_t>(static_cast<uint64_t>(spec.tv_sec) * 1000 + static_cast<uint64_t>(spec.tv_nsec) / 1000000);
    }
#endif
}
