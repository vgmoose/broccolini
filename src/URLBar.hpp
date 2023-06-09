#pragma once

#include "../libs/chesto/src/Element.hpp"
#include "WebView.hpp"
#include <string>

class URLBar : public Element
{
public:
	URLBar(WebView* webView);

    WebView* webView = NULL;

    // bool process(InputEvents* event);
    // void render(Element* parent);
};
