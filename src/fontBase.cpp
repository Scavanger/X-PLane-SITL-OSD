#include "fontBase.h"

std::string FontBase::getName()
{
    return this->name;
}

unsigned int FontBase::getCharWidth()
{
    return this->charWidth;
}

unsigned int FontBase::getCharHeight()
{
    return this->charHeight;
}

std::vector<std::vector<uint8_t>> FontBase::getTextures()
{
    return this->textures;
}
