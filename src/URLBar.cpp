#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/Constraint.hpp"

URLBar::URLBar(WebView* webView)
{
    this->webView = webView;

    this->width = RootDisplay::screenWidth;
    this->height = 30;

    this->isAbsolute = true;

   CST_Color gray = { 80, 80, 80, 0xff };
    auto urlText = new TextElement(webView->url.c_str(), 30, &gray);
    urlText->constrain(ALIGN_CENTER_HORIZONTAL, 0);
    child(urlText);
}

// bool process(InputEvents* event) {
//     return true;
// }

// void render(Element* parent) {

// }