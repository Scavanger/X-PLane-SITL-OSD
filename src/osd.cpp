#include "osd.h"

#include "helper.h"

using namespace Helper;

#include <filesystem>

typedef enum {
    DP_SUB_CMD_CLEAR_SCREEN = 2,
    DP_SUB_CMD_WRITE_STRING = 3,
    DP_SUB_CMD_DRAW_SCREEN = 4,
    DP_SUB_CMD_SET_OPTIONS = 5
} mspDisplayportSubCmd_t;

OSD::OSD()
{
    this->osdRenderer = std::make_unique<OsdRenderer>();
    this->fontsHDZero = std::vector<std::shared_ptr<FontHDZero>>();
    this->fontsWtfOs = std::vector<std::shared_ptr<FontWtfOS>>();
    this->fontsWalksnail = std::vector<std::shared_ptr<FontWalksnail>>();
    for (std::filesystem::path path : getFontPaths("wtfos", true)) {
        this->fontsWtfOs.push_back(std::make_shared<FontWtfOS>(path));
    }
    
    for (std::filesystem::path path : getFontPaths("hdzero", false)) {
        this->fontsHDZero.push_back(std::make_shared<FontHDZero>(path));
    }

    for (std::filesystem::path path : getFontPaths("walksnail", false)) {
        this->fontsWalksnail.push_back(std::make_shared<FontWalksnail>(path));
    }

    this->setDefaultFonts();
}

OSD::~OSD()
{
    this->activeHDZeroFont.reset();
    this->activeWalksnailFont.reset();
    this->activeWtfOsFont.reset();
}

void OSD::setVideoSystem(videoSystem_e system)
{
    this->videoSystem = system;
    switch (system) {
        case VIDEO_SYSTEM_HDZERO:
            this->actualRows = HDZERO_ROWS;
            this->actualCols = HDZERO_COLS;
            if (!this->fontsHDZero.empty()) {
                this->osdRenderer->LoadFont(this->activeHDZeroFont);
            }
            break;
        case VIDEO_SYSTEM_WALKSNAIL:
            this->actualRows = WALKSNAIL_ROWS;
            this->actualCols = WALKSNAIL_COLS;
            if (!this->fontsWalksnail.empty()) {
                this->osdRenderer->LoadFont(this->activeWalksnailFont);
            }
            break;
        case VIDEO_SYSTEM_WTFOS:
            this->actualRows = DJI_ROWS;
            this->actualCols = DJI_COLS;
            if (!this->fontsWtfOs.empty()) {
                this->osdRenderer->LoadFont(this->activeWtfOsFont);
            }
            break;
        default:
            this->actualRows = this->actualCols = 0;
    }
}


std::vector<std::string> OSD::getWtfFontNames()
{
    std::vector<std::string> names = std::vector<std::string>();
    for (std::shared_ptr<FontWtfOS> font : this->fontsWtfOs) {
        names.push_back(font->getName());    
    }

    return names;
}

std::vector<std::string> OSD::getHDZeroFontNames()
{
    std::vector<std::string> names = std::vector<std::string>();
    for (std::shared_ptr<FontHDZero> font : this->fontsHDZero) {
        names.push_back(font->getName());
    }
    return names;
}

std::vector<std::string> OSD::getWalksnailFontNames()
{
    std::vector<std::string> names = std::vector<std::string>();
    for (std::shared_ptr<FontWalksnail> font : this->fontsWalksnail) {
        names.push_back(font->getName());
    }
    return names;
}

void OSD::registerOnVideoSysteChangedCb(std::function<void(videoSystem_e)> callback)
{
    if (callback) {
        this->onVideoSystemChanged = callback;
    }
}

videoSystem_e OSD::getVideoSystem()
{
    return this->videoSystem;
}

std::string OSD::getActiveWfosFontName()
{
    return this->activeWtfOsFont->getName();
}

std::string OSD::getActiveWalksnailFontName()
{
    return this->activeWalksnailFont->getName();
}

std::string OSD::getActiveHDZeroFontName()
{
    return this->activeHDZeroFont->getName();
}

void OSD::decode(mspCommand_e cmd, std::vector<uint8_t> data)
{
    if (data.empty()) {
        return;
    }

    switch (cmd) {
        case MSP2_INAV_OSD_PREFERENCES:
        {    
            if ((data[0] < VIDEO_SYSTEM_HDZERO || data[0] > VIDEO_SYSTEM_WALKSNAIL)) {
                Log("Warning: Unsuported video system detected, fallback to WtfOS.");
                this->setVideoSystem(VIDEO_SYSTEM_WTFOS);
            } else {
                this->setVideoSystem((videoSystem_e)data[0]);
            }
            this->onVideoSystemChanged(this->videoSystem);
            break;
        }
        case MSP_DISPLAYPORT:
        {
            mspDisplayportSubCmd_t subCmd = static_cast<mspDisplayportSubCmd_t>(data[0]);
            if (subCmd == DP_SUB_CMD_CLEAR_SCREEN) {
                this->osdRenderer->clearScreen();
                break;
            } else if (subCmd == DP_SUB_CMD_WRITE_STRING) {
                if (data.size() < 5) {
                    break;
                }
                uint8_t row = data[1];
                uint8_t col = data[2];
                bool isExtdChar = data[3];
                for (size_t i = 4; i < data.size(); i++) {
                    this->osdRenderer->setCharacter(row, col + i - 4, isExtdChar ? (static_cast<uint16_t>(data[i]) | 0x100) : data[i]);
                }
            }
            break;
        }
        default:
            break;
    }
}

void OSD::setActiveFont(std::string name)
{
    videoSystem_e fontSystem = VIDEO_SYSTEM_NONE;
    for (std::shared_ptr<FontWtfOS> font : this->fontsWtfOs) {
        if (font->getName() == name) {
            this->activeWtfOsFont = font;
            fontSystem = VIDEO_SYSTEM_WTFOS;
        }    
    }

    for (std::shared_ptr<FontHDZero> &font : this->fontsHDZero) {
        if (font->getName() == name) {
            this->activeHDZeroFont = font;
            fontSystem = VIDEO_SYSTEM_HDZERO;
        }    
    }

    for (std::shared_ptr<FontWalksnail> font : this->fontsWalksnail) {
        if (font->getName() == name) {
            this->activeWalksnailFont = font;
            fontSystem = VIDEO_SYSTEM_WALKSNAIL;
        }    
    }

    if (fontSystem == this->videoSystem) {
        this->setVideoSystem(fontSystem);
    } 
}
void OSD::setDefaultFonts()
{
    if (!this->fontsHDZero.empty()) {
        this->activeHDZeroFont = this->fontsHDZero[0];
    }

    if (!this->fontsWalksnail.empty()) {
        this->activeWalksnailFont = this->fontsWalksnail[0];
    }

    if (!this->fontsWtfOs.empty()) {
        this->activeWtfOsFont = this->fontsWtfOs[0];
        this->setVideoSystem(VIDEO_SYSTEM_WTFOS);
    }
}

void OSD::clear()
{
    this->osdRenderer->clearScreen();
}

void OSD::draw()
{
    if (this->showToast && getTickCount() > this->toastEndTime) {
        this->osdRenderer->clearScreen();
        this->showToast = false;
    }

    this->osdRenderer->render(this->actualRows, this->actualCols);
}

void OSD::makeToast(std::string msg, int durationMs)
{
    this->showToast = true;;
    this->toastEndTime = getTickCount() + durationMs;
    this->osdRenderer->clearScreen();
    int startCol = this->actualCols / 2 - msg.length() / 2;
    int len = msg.length();
    if (startCol + msg.length() >= this->actualCols) {
        len = this->actualCols - startCol;
    }
    for (int i = 0; i < len; i++) {
        this->osdRenderer->setCharacter(2, startCol + i, static_cast<uint16_t>(msg[i]));
    }
}
