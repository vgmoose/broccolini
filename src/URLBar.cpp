#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/Constraint.hpp"

URLBar::URLBar(WebView* webView)
{
    this->webView = webView;

    this->width = RootDisplay::screenWidth;
    this->height = 75;

    this->isAbsolute = true;

    CST_Color gray = { 80, 80, 80, 0xff };

    urlText = new TextElement(webView->url.c_str(), 24, &gray);
    urlText->constrain(ALIGN_CENTER_HORIZONTAL, 0);
    urlText->constrain(ALIGN_CENTER_VERTICAL, 0);
    child(urlText);
}

void URLBar::updateInfo() {
    urlText->setText(webView->url);
    urlText->update();
}

// bool URLBar::process(InputEvents* event) {
//     return true;
// }

void URLBar::render(Element* parent) {
    // Draw background

    CST_Rect rect = { 0, 0, width, height };
    CST_SetDrawColorRGBA(RootDisplay::renderer, 0xee, 0xee, 0xee, 0xff);
    CST_FillRect(RootDisplay::renderer, &rect);

    auto innerWidth = width * 0.8;
    auto innerHeight = height * 0.6;

    CST_roundedBoxRGBA(RootDisplay::renderer, width/2 -  innerWidth/2, height/2 - innerHeight/2, width/2 +  innerWidth/2, height/2 + innerHeight/2, 10, 0xdd, 0xdd, 0xdd, 0xff);

    Element::render(parent);
}