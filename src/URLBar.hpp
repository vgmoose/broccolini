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

    std::string currentUrl = "";

    WebView* webView = NULL;
    TextElement* urlText = NULL;
    TabSwitcher* tabSwitcher = NULL;

    bool highlightingKeyboard = false;
    bool themeIsTooDark = false;

    Element* forwardButton = NULL;
    Element* clockButton = NULL;
    Element* bookmarkButton = NULL;
    Element* privateIndicator = NULL;

    EKeyboard* keyboard = NULL;

    void showKeyboard();
    void resetBar();
    void updateVisibleUrlText();
    void updateIconMaskColors(bool fillWhite);
    void saveCurTabScreenshot(bool isPrivate = false);
    void createURLBarElements();
    Element* makeURLBarButton(std::string iconPath, std::function<bool()> action);

    bool process(InputEvents* event);
    void render(Element* parent);

    CST_Color getThemeColor();
};
