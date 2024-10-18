#include "platform.h"
#include "osdPlugin.h"

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    return OsdPlugin::instance()->start(outName, outSig, outDesc);
}

PLUGIN_API void	XPluginStop(void)
{
    OsdPlugin::instance().reset();
}

PLUGIN_API int XPluginEnable(void)
{
    return OsdPlugin::instance()->enable();
}

PLUGIN_API void XPluginDisable(void)
{
    return OsdPlugin::instance()->disable();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam) 
{   
    OsdPlugin::instance()->xPluginReceiveMessage(inFrom, inMsg, inParam);
}
