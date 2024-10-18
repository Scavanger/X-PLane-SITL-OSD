#pragma once

#include "../platform.h"

#include <string>
#include <functional>
#include <memory>
#include <XPWidgetDefs.h>

class IPInputWidget
{
    private:
        XPWidgetID widget = NULL;
        XPWidgetID textField = NULL;
        XPWidgetID okButton = NULL;
        XPWidgetID cancelButton = NULL;
        bool shown = false;
        bool isInitalized = false;
        std::function<void(std::string)> valueChanged = nullptr;

        static int widgetMessageStatic(XPWidgetMessage inMessage, XPWidgetID  inWidget, intptr_t inParam1, intptr_t  inParam2);
        int widgetMessage(XPWidgetMessage inMessage, XPWidgetID  inWidget, intptr_t inParam1, intptr_t  inParam2);
        void close();

    public:
        
        IPInputWidget() = default;
        
        static std::shared_ptr<IPInputWidget> instance();
        IPInputWidget(IPInputWidget const&) = delete;
        IPInputWidget& operator =(IPInputWidget const&) = delete;

        void create(int x, int y, int width, int height, std::string desc, std::string caption);
        void show();
        void registerValueChangedCb(std::function<void(std::string)> callback);
        std::string getValue();
        void setValue(std::string value);
};
