#pragma once

#include "fontBase.h"

class FontWalksnail : public FontBase {
    public:
        FontWalksnail() = default;
        FontWalksnail(std::filesystem::path path);
        int getCols();
        int getRows();
};