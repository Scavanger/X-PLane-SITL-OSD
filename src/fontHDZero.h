#pragma once

#include "fontBase.h"

class FontHDZero : public FontBase {
    public:
        FontHDZero() = default;
        FontHDZero(std::filesystem::path path);
        int getCols();
        int getRows();
};