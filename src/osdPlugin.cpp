
#include <string.h>
#include <functional>
#include <filesystem>

#include "osdPlugin.h"
#include "helper.h"

using namespace Helper;
using namespace std::placeholders;
using namespace std::filesystem;

OsdPlugin::OsdPlugin()
{
    XPLMRegisterDrawCallback(&staticDrawCallback, xplm_Phase_Window, 0, NULL);
    this->msp = std::make_unique<MSP>();
    this->osd = std::make_unique<OSD>();
}

OsdPlugin::~OsdPlugin()
{
    XPLMUnregisterDrawCallback(&staticDrawCallback, xplm_Phase_Window, 0, NULL);
}

std::shared_ptr<OsdPlugin> OsdPlugin::instance()
{
    static std::shared_ptr<OsdPlugin> instance = std::make_shared<OsdPlugin>();
    return instance;
}

int OsdPlugin::start(char *outName, char *outSig, char *outDesc)
{
    Log("INAV SITL OSD Plugin start");
    
    strcpy(outName, NAME.c_str());
	strcpy(outSig, SIGNATURE.c_str());
	strcpy(outDesc, DESCRIPTION.c_str());    
    
    return 1;
}

int OsdPlugin::enable() 
{
    menu = Menu::instance();
    menu->setFontMenu(VIDEO_SYSTEM_HDZERO, osd->getHDZeroFontNames());
    menu->setFontMenu(VIDEO_SYSTEM_WALKSNAIL, osd->getWalksnailFontNames());
    menu->setFontMenu(VIDEO_SYSTEM_WTFOS, osd->getWtfFontNames());

    ipInputWidget = IPInputWidget::instance();
    
    XPLMCreateFlightLoop_t params;
    params.structSize = sizeof(XPLMCreateFlightLoop_t);
    params.callbackFunc = &staticFlightLoopCb;
    params.phase = xplm_FlightLoop_Phase_AfterFlightModel;
    params.refcon = NULL;
    this->flLoopId = XPLMCreateFlightLoop(&params);
    XPLMScheduleFlightLoop(this->flLoopId, -1, true);

    ipInputWidget->create(300, 100, 300, 105, "IP Address", "IP Address");
    ipInputWidget->registerValueChangedCb(std::bind(&OsdPlugin::ipAddressChanged, this, _1));

    menu->registerOnConnectCb(std::bind(&OsdPlugin::connect, this));
    menu->registerOnPortChangedCb(std::bind(&OsdPlugin::portChanged, this, _1));
    menu->registerOnFontChangedCb(std::bind(&OsdPlugin::fontChanged, this, _1));
    msp->registerMessageReceivedCb(std::bind(&OsdPlugin::mspMessageReveiced, this, _1, _2));
    msp->registerDisconnectCb(std::bind(&OsdPlugin::disconnect, this));
    osd->registerOnVideoSysteChangedCb(std::bind(&OsdPlugin::videoSystemChanged, this, _1));

    this->loadConfig();
    menu->setPort(this->port);
    menu->setIpAddress(this->ipAddress);
    menu->setActiveFonts(osd->getActiveHDZeroFontName(), osd->getActiveWalksnailFontName(), osd->getActiveWfosFontName());
    
    return 1;
}

void OsdPlugin::disable()
{
    menu->destroy();
    XPLMDestroyFlightLoop(this->flLoopId);
}

void OsdPlugin::xPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void *inParam)
{
    if (inMsg == XPLM_MSG_AIRPORT_LOADED) {
        this->osd->makeToast(PLUGIN_NAME + " V" + PLUGIN_VERSION, 5000);
    }
}

float OsdPlugin::flightLoopCb(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    if (msp->isConnected() && getTickCount() > this->timeSinceLastLoop + LOOP_TIME) {
        this->timeSinceLastLoop = getTickCount();
        msp->receive();
        msp->send(MSP_FC_VARIANT);
    }
    return -1;
}

int OsdPlugin::drawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
    this->osd->draw();
    return 1;
}

void OsdPlugin::mspMessageReveiced(mspCommand_e cmd, std::vector<uint8_t> buffer)
{
    osd->decode(cmd, buffer);
}

void OsdPlugin::connect()
{
    if (!this->msp->isConnected()) {
        this->msp->connect(this->ipAddress, this->port);
        
        if (this->msp->isConnected()) {
            this->timeSinceLastLoop = getTickCount();
            menu->enbaleMenu(VIDEO_SYSTEM_NONE, false); 
            // Determ video system
            this->msp->send(MSP2_INAV_OSD_PREFERENCES);
        }
    } else {
        this->osd->clear();
        this->msp->disconnect();
    }
    this->menu->setConnected(this->msp->isConnected());
}

void OsdPlugin::disconnect()
{
    this->menu->setConnected(false);
    menu->enbaleMenu(VIDEO_SYSTEM_NONE, true);
    this->osd->makeToast("DISCONNECTED", 3000);
}

void OsdPlugin::fontChanged(std::string font)
{
    osd->setActiveFont(font);
    this->saveConfig();
    menu->setActiveFonts(osd->getActiveHDZeroFontName(), osd->getActiveWalksnailFontName(), osd->getActiveWfosFontName());
}

void OsdPlugin::portChanged(int port)
{
   this->port = port;
   this->saveConfig();
}

void OsdPlugin::ipAddressChanged(std::string ipAddress)
{
    this->ipAddress = ipAddress;
    this->saveConfig();
}

void OsdPlugin::videoSystemChanged(videoSystem_e videoSystem)
{
    this->menu->enbaleMenu(videoSystem, false);
}

void OsdPlugin::loadConfig()
{
    path path = getConfigFileName();
    mINI::INIFile config(path.generic_string());
    if (config.read(this->ini)) {

        if (this->ini[INI_CONFIG].has(INI_PORT)) {
            this->port = std::stoi(this->ini[INI_CONFIG][INI_PORT]);
        }

        if (this->ini[INI_CONFIG].has(INI_IP)) {
            this->ipAddress = this->ini[INI_CONFIG][INI_IP];
        }

        if (this->ini[INI_CONFIG].has(HDZERO_FONT)) {
            osd->setActiveFont(this->ini[INI_CONFIG][HDZERO_FONT]);
        }

        if (this->ini[INI_CONFIG].has(WALKSNAIL_FONT)) {
            osd->setActiveFont(this->ini[INI_CONFIG][WALKSNAIL_FONT]);
        }

        if (this->ini[INI_CONFIG].has(WTFOS_FONT)) {
            osd->setActiveFont(this->ini[INI_CONFIG][WTFOS_FONT]);
        }
    } else {
        Log("Warning: Unable to read config, using default values.");
    }
}

void OsdPlugin::saveConfig()
{
    this->ini[INI_CONFIG][INI_PORT] = std::to_string(this->port);
    this->ini[INI_CONFIG][INI_IP] = this->ipAddress;
    this->ini[INI_CONFIG][HDZERO_FONT] = osd->getActiveHDZeroFontName();
    this->ini[INI_CONFIG][WALKSNAIL_FONT] = osd->getActiveWalksnailFontName();
    this->ini[INI_CONFIG][WTFOS_FONT] = osd->getActiveWfosFontName();

    path path = getConfigFileName();
    mINI::INIFile config(path.generic_string());
    if (!config.generate(this->ini, true)) {
        Log("Warning: Unable to save config.");
    }
}

float OsdPlugin::staticFlightLoopCb(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    return instance()->flightLoopCb(inElapsedSinceLastCall, inElapsedTimeSinceLastFlightLoop, inCounter, inRefcon);
}

int OsdPlugin::staticDrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
    return instance()->drawCallback(inPhase, inIsBefore, inRefcon);
}

