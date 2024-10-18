#pragma once

#include <memory>
#include <functional>

#include <XPLMMenus.h>

#include "fontBase.h"
#include "osd.h"

class Menu {
    
    private:
        int menuIdx;
        int connectItemIdx;
        XPLMMenuID menuId;

        int portMenuIdx;
        std::vector<uint8_t> portIdx;
        XPLMMenuID portMenuId;
        int ipAdressMenuIdx;

        int fontMenuIdx;
        XPLMMenuID fontMenuId;

        int fontMenuHDZeroIdx;
        XPLMMenuID fontMenuHDZeroId;
        std::vector<int> fontItemsHDZeroIdx;

        int fontMenuWalksnailIdx;
        XPLMMenuID fontMenuWalksnailId;
        std::vector<int> fontItemsWalksnailIdx;
        

        int fontMenuWtfOsIdx;
        XPLMMenuID fontMenuWtfOsId;
        std::vector<int> fontItemsWtfOsIdx;
        
        std::vector<std::pair<std::string, std::pair<XPLMMenuID, int>>> fontEntries;

        std::function<void(void)> onConnect;
        std::function<void(int)> onPortChanged;
        std::function<void(std::string)> onFontChanged;

        static void staticMenuHandler(void * in_menu_ref, void * in_item);       
    
    public:
        Menu();
        ~Menu();

        static std::shared_ptr<Menu> instance();

        Menu(Menu const&) = delete;
        Menu& operator =(Menu const&) = delete;

        void setPort(int port);
        void setIpAddress(std::string ipAddress);
        void setConnected(bool isConnected);
        void setActiveFonts(std::string hdZeroFont, std::string walksnailFont, std::string wtfOsFont);
        void enbaleMenu(videoSystem_e videoSystem, bool enable);
        void setFontMenu(videoSystem_e videoSystem, std::vector<std::string> items);
        void registerOnConnectCb(std::function<void(void)> callback);
        void registerOnPortChangedCb(std::function<void(int)> callback);
        void registerOnFontChangedCb(std::function<void(std::string)> callback);
        void menuHandler(void * in_menu_ref, void * in_item);
        void destroy();        
};