#pragma once

#include "platform.h"

#include <memory>
#include <XPLMDisplay.h>
#include <XPLMProcessing.h>

#include "mini/ini.h"
#include "menu.h"
#include "msp.h"
#include "osd.h"
#include "widgets/ipInputWidget.h"

const std::string NAME = "INAV SITL OSD";
const std::string SIGNATURE  = "scavanger.inav.xplane.sitl-osd";
const std::string DESCRIPTION = "INAV OSD for SITL";

const int STANDARD_PORT = 5760;
const std::string STANDRD_IP = "127.0.0.1";

const std::string INI_CONFIG      = "config";
const std::string INI_PORT        = "port";
const std::string INI_IP          = "ip";
const std::string WALKSNAIL_FONT  = "walksnail_font";
const std::string HDZERO_FONT     = "hdzero_font";
const std::string WTFOS_FONT      = "wtfos_font";

const uint LOOP_TIME = 125; // ms
const std::string PLUGIN_NAME = "INAV SITL OSD PLUGIN";
const std::string PLUGIN_VERSION = "0.1";

class OsdPlugin {
    private:
        std::shared_ptr<Menu> menu;
        std::shared_ptr<IPInputWidget> ipInputWidget;
        std::unique_ptr<OSD> osd;
        XPLMFlightLoopID flLoopId;
        mINI::INIStructure ini;
        std::unique_ptr<MSP> msp;
        uint32_t timeSinceLastLoop = 0;

        int port = STANDARD_PORT;
        std::string ipAddress = STANDRD_IP;

        static float staticFlightLoopCb(
                         float inElapsedSinceLastCall,    
                         float inElapsedTimeSinceLastFlightLoop,    
                         int   inCounter,    
                         void *inRefcon); 

        float flightLoopCb(
                         float inElapsedSinceLastCall,    
                         float inElapsedTimeSinceLastFlightLoop,    
                         int   inCounter,    
                         void *inRefcon);   
        
        static int staticDrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon);
        int drawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon);

        void mspMessageReveiced(mspCommand_e cmd, std::vector<uint8_t> buffer);
        void connect();
        void disconnect();
        void fontChanged(std::string font);
        void portChanged(int port);
        void ipAddressChanged(std::string ipAddress);
        void videoSystemChanged(videoSystem_e videoSystem);
        void loadConfig();
        void saveConfig();
    
    public:
        OsdPlugin();
        ~OsdPlugin();

        static std::shared_ptr<OsdPlugin> instance();
        OsdPlugin(OsdPlugin const&) = delete;
        OsdPlugin& operator =(OsdPlugin const&) = delete;

        int enable();
        void disable();
        int start(char *outName, char *outSig, char *outDesc);
        void xPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam);
};
