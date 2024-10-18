#include "menu.h"

#include <string.h>
#include <cstddef>

#include "widgets/ipInputWidget.h"

#define SITL_PORT_COUNT 8
#define SITL_FIRST_PORT 5760

#define MAKE_MENU_REF(ref)      ((void*)(size_t)ref)
#define IS_MENU_REF(item, ref)  ((size_t)item == (size_t)ref)

#define MENU_REF_PLUGIN             MAKE_MENU_REF(0x10)
#define MENU_REF_PORTS              MAKE_MENU_REF(0x11)
#define MENU_REF_FONTS              MAKE_MENU_REF(0x12)
#define MENU_REF_FONT               MAKE_MENU_REF(0x13)
#define MENU_ITEM_REF_CONNECT       MAKE_MENU_REF(0x01)
#define MENU_ITEM_REF_IP_ADDRESS    MAKE_MENU_REF(0x02)

Menu::Menu()
{
    this->fontEntries = std::vector<std::pair<std::string, std::pair<XPLMMenuID, int>>>();
    this->fontItemsHDZeroIdx = std::vector<int>();
    this->fontItemsWalksnailIdx = std::vector<int>();

    this->menuIdx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "INAV SITL OSD", 0, 0);
    this->menuId = XPLMCreateMenu("INAV SITL OSD", XPLMFindPluginsMenu(), this->menuIdx, &this->staticMenuHandler, MENU_REF_PLUGIN);
    this->connectItemIdx = XPLMAppendMenuItem(this->menuId, "Connect", MENU_ITEM_REF_CONNECT, 0);

    this->portMenuIdx = XPLMAppendMenuItem(this->menuId, "Port", 0, 0);
    this->portMenuId = XPLMCreateMenu("Port", this->menuId, this->portMenuIdx, &this->staticMenuHandler, MENU_REF_PORTS);    
    this->portIdx = std::vector<uint8_t>(SITL_PORT_COUNT);
    for (int i = 0; i < SITL_PORT_COUNT; i++) {
        int port = SITL_FIRST_PORT + i;
        std::string name = "Port " + std::to_string(port) + " (UART " + std::to_string(i + 1) + ")";
        this->portIdx[i] = XPLMAppendMenuItem(this->portMenuId, name.c_str(), MAKE_MENU_REF(port), 0);
    }

    this->ipAdressMenuIdx = XPLMAppendMenuItem(this->menuId, "IP Address", MAKE_MENU_REF(MENU_ITEM_REF_IP_ADDRESS), 0);

    this->fontMenuIdx = XPLMAppendMenuItem(this->menuId, "Fonts", 0, 0);
    this->fontMenuId = XPLMCreateMenu("Fonts", this->menuId, this->fontMenuIdx, &this->staticMenuHandler, MENU_REF_FONTS);

    this->fontMenuHDZeroIdx = XPLMAppendMenuItem(this->fontMenuId, "HD Zero", 0, 0);
    this->fontMenuHDZeroId = XPLMCreateMenu("HD Zero", this->fontMenuId, this->fontMenuHDZeroIdx, &this->staticMenuHandler, MENU_REF_FONT);

    this->fontMenuWalksnailIdx = XPLMAppendMenuItem(this->fontMenuId, "Walksnail", 0, 0);
    this->fontMenuWalksnailId = XPLMCreateMenu("Walksnail", this->fontMenuId, this->fontMenuWalksnailIdx, &this->staticMenuHandler, MENU_REF_FONT);

    this->fontMenuWtfOsIdx = XPLMAppendMenuItem(this->fontMenuId, "WTF OS", 0, 0);
    this->fontMenuWtfOsId = XPLMCreateMenu("WTF OS", this->fontMenuId, this->fontMenuWtfOsIdx, &this->staticMenuHandler, MENU_REF_FONT);
}

Menu::~Menu()
{
    this->destroy();
}

std::shared_ptr<Menu> Menu::instance()
{
    static std::shared_ptr<Menu> instance = std::make_shared<Menu>();
    return instance;
}

void Menu::setPort(int port)
{
    int checkedIdx = port - SITL_FIRST_PORT;
    for (uint8_t id : this->portIdx) {
        XPLMCheckMenuItem(this->portMenuId, id, checkedIdx == id ? xplm_Menu_Checked: xplm_Menu_Unchecked);
    }
}

void Menu::setIpAddress(std::string ipAddress)
{
    IPInputWidget::instance()->setValue(ipAddress);
}

void Menu::setConnected(bool isConnected)
{
    XPLMSetMenuItemName(this->menuId, this->connectItemIdx, isConnected ? "Disconnect" : "Connect", 0);
}

void Menu::setActiveFonts(std::string hdZeroFont, std::string walksnailFont, std::string wtfOsFont)
{
    for (std::pair<std::string, std::pair<XPLMMenuID, int>> item : this->fontEntries) {
        XPLMMenuCheck checked = xplm_Menu_Unchecked;
        if (item.first == hdZeroFont || item.first == walksnailFont || item.first == wtfOsFont) {
            checked = xplm_Menu_Checked;
        }
        XPLMCheckMenuItem(item.second.first, item.second.second, checked);   
    }
}

void Menu::enbaleMenu(videoSystem_e videoSystem, bool enable)
{
    XPLMEnableMenuItem(this->menuId, this->portMenuIdx, enable);
    for (uint8_t id: this->portIdx) {
        XPLMEnableMenuItem(this->portMenuId, id, enable);
    }
    XPLMEnableMenuItem(this->menuId, this->ipAdressMenuIdx, enable);

    bool enableHDZero = true, enableWalksnail = true, enableWtfOs = true;
    if (!enable) {
        switch (videoSystem) {
            case VIDEO_SYSTEM_HDZERO:
                enableWalksnail = false;
                enableWtfOs = false;
                break;
            case VIDEO_SYSTEM_WALKSNAIL:
                enableHDZero = false;
                enableWtfOs = false;
                break;
            case VIDEO_SYSTEM_WTFOS:
                enableHDZero = false;
                enableWalksnail = false;
                break;
            default:
                break;
        }
    }

    XPLMEnableMenuItem(this->fontMenuId, this->fontMenuHDZeroIdx, enableHDZero);
    for (int idx : this->fontItemsHDZeroIdx) {
        XPLMEnableMenuItem(this->fontMenuHDZeroId, idx, enableHDZero);
    }

    XPLMEnableMenuItem(this->fontMenuId, this->fontMenuWalksnailIdx, enableWalksnail);
    for (int idx : this->fontItemsWalksnailIdx) {
        XPLMEnableMenuItem(this->fontMenuWalksnailId, idx, enableWalksnail);
    }

    XPLMEnableMenuItem(this->fontMenuId, this->fontMenuWtfOsIdx, enableWtfOs);
    for (int idx : this->fontItemsWtfOsIdx) {
        XPLMEnableMenuItem(this->fontMenuWtfOsId, idx, enableWtfOs);
    }    
}

void Menu::setFontMenu(videoSystem_e videoSystem, std::vector<std::string> items)
{
    int idx = -1;
    XPLMMenuID id;
    std::vector<int> *fontItemsIdx;
    switch (videoSystem) {
        case VIDEO_SYSTEM_HDZERO:
            idx = this->fontMenuHDZeroIdx;
            id = this->fontMenuHDZeroId;
            fontItemsIdx = &this->fontItemsHDZeroIdx;
            break;
        case VIDEO_SYSTEM_WALKSNAIL:
            idx = this->fontMenuWalksnailIdx;
            id = this->fontMenuWalksnailId;
            fontItemsIdx = &this->fontItemsWalksnailIdx;
            break;
        case VIDEO_SYSTEM_WTFOS:
            idx = this->fontMenuWtfOsIdx;
            id = this->fontMenuWtfOsId;
            fontItemsIdx = &this->fontItemsWtfOsIdx;
            break;
        default:
            break;
    }

    if (idx == -1) {
        return;
    }

    for (std::string item : items) {
        int idx = XPLMAppendMenuItem(id, item.c_str(), (void*)(this->fontEntries.size()), 0);
        fontItemsIdx->push_back(idx);
        this->fontEntries.push_back({item, {id, idx}});
    }
}

void Menu::registerOnConnectCb(std::function<void(void)> callback)
{
    if (callback) {
        this->onConnect = callback;
    }
}

void Menu::registerOnPortChangedCb(std::function<void(int)> callback)
{
    if (callback) {
        this->onPortChanged = callback;
    }
}

void Menu::registerOnFontChangedCb(std::function<void(std::string)> callback)
{
    if (callback) {
        this->onFontChanged = callback;
    }
}

void Menu::menuHandler(void *in_menu_ref, void *in_item)
{
    if (IS_MENU_REF(in_menu_ref, MENU_REF_PORTS)) {
        int port = (size_t)in_item;
        this->setPort(port);
        this->onPortChanged(port);
    } else if (IS_MENU_REF(in_menu_ref, MENU_REF_PLUGIN)) {
        if (IS_MENU_REF(in_item, MENU_ITEM_REF_CONNECT)) {
            this->onConnect();
        } else if (IS_MENU_REF(in_item, MENU_ITEM_REF_IP_ADDRESS)) {
            IPInputWidget::instance()->show();
        }
    } else if (IS_MENU_REF(in_menu_ref, MENU_REF_FONT)) {
        size_t idx = (size_t)in_item;
        if (idx < this->fontEntries.size()) {
            this->onFontChanged(this->fontEntries[idx].first);
        }
    }
}

void Menu::destroy()
{
    XPLMDestroyMenu(this->fontMenuWalksnailId);
    XPLMDestroyMenu(this->fontMenuWtfOsId);
    XPLMDestroyMenu(this->fontMenuHDZeroId);
    XPLMDestroyMenu(this->fontMenuId);
    XPLMDestroyMenu(this->portMenuId);
    XPLMDestroyMenu(this->menuId);
}

void Menu::staticMenuHandler(void *in_menu_ref, void *in_item)
{
    instance()->menuHandler(in_menu_ref, in_item);
}
