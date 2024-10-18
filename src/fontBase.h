#pragma once

#include "platform.h"

#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>

#define BYTES_PER_PIXEL_RGBA 4

class FontBase {
    
    protected:
        std::vector<std::vector<uint8_t>> textures;
        std::string name;
        unsigned int charWidth = 0;
        unsigned int charHeight = 0;

    private:
        
    
    public:
        std::string getName();
        unsigned int getCharWidth();
        unsigned int getCharHeight();
        std::vector<std::vector<uint8_t>> getTextures();
        virtual int getCols() = 0;
        virtual int getRows() = 0;
    };