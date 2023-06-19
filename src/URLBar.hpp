#pragma once

#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/EKeyboard.hpp"
#include "WebView.hpp"
#include "ClockElement.hpp"
#include <string>

class TabSwitcher;

class URLBar : public Element
{
public:
	URLBar(WebView* webView);
    void updateInfo();

    WebView* webView = NULL;
    TextElement* urlText = NULL;
    TabSwitcher* tabSwitcher = NULL;

    bool highlightingKeyboard = false;

    Element* forwardButton = NULL;
    Element* clockButton = NULL;

    EKeyboard* keyboard = NULL;

    void showKeyboard();
    void resetBar();
    void saveCurTabScreenshot();

    bool process(InputEvents* event);
    void render(Element* parent);
};
