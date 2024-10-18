#pragma once

#include "platform.h"

#include <vector>
#include <string>
#include "msp.h"
#include "osdRenderer.h"
#include "fontWtfOs.h"
#include "fontHDZero.h"
#include "fontWalksnail.h"

static const uint HDZERO_COLS       = 50;
static const uint HDZERO_ROWS       = 18;
static const uint WALKSNAIL_COLS    = 53;
static const uint WALKSNAIL_ROWS    = 20;
static const uint DJI_COLS          = 60;
static const uint DJI_ROWS          = 22;

typedef enum {
    VIDEO_SYSTEM_HDZERO     = 3,
    VIDEO_SYSTEM_WTFOS      = 4,      
    VIDEO_SYSTEM_WALKSNAIL  = 5,
    VIDEO_SYSTEM_NONE       = 6,
} videoSystem_e;

class OSD {
    
    private:    
        std::unique_ptr<OsdRenderer> osdRenderer;
        
        std::vector<std::shared_ptr<FontHDZero>> fontsHDZero;
        std::vector<std::shared_ptr<FontWtfOS>> fontsWtfOs;
        std::vector<std::shared_ptr<FontWalksnail>> fontsWalksnail;

        std::shared_ptr<FontHDZero> activeHDZeroFont;
        std::shared_ptr<FontWalksnail> activeWalksnailFont;
        std::shared_ptr<FontWtfOS> activeWtfOsFont;
    
        videoSystem_e videoSystem = VIDEO_SYSTEM_NONE;

        std::function<void(videoSystem_e)> onVideoSystemChanged;

        int actualRows = 0;
        int actualCols = 0;

        uint32_t toastEndTime = 0;
        bool showToast = false;

        void setVideoSystem(videoSystem_e system);
    
    public:
        OSD();
        ~OSD();

        std::vector<std::string> getWtfFontNames();
        std::vector<std::string> getHDZeroFontNames();
        std::vector<std::string> getWalksnailFontNames();

        void registerOnVideoSysteChangedCb(std::function<void(videoSystem_e)> callback);
        
        videoSystem_e getVideoSystem();
        std::string getActiveWfosFontName();
        std::string getActiveWalksnailFontName();
        std::string getActiveHDZeroFontName();
        void decode(mspCommand_e cmd, std::vector<uint8_t> data);
        void setActiveFont(std::string name);
        void setDefaultFonts();
        void clear();
        void draw();
        void makeToast(std::string msg, int durationMs);

};