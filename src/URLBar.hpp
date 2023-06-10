#pragma once

#include "../libs/chesto/src/TextElement.hpp"
#include "WebView.hpp"
#include <string>

class URLBar : public Element
{
public:
	URLBar(WebView* webView);
    void updateInfo();

    WebView* webView = NULL;
    TextElement* urlText = NULL;

    // bool process(InputEvents* event);
    void render(Element* parent);
};
