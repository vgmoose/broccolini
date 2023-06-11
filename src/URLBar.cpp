#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/Constraint.hpp"
#include "../libs/chesto/src/EKeyboard.hpp"

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

    int iconSize = 30;

    // set up icons
    child(
        (new ImageElement(RAMFS "res/icons/back.svg"))
        ->setSize(iconSize, iconSize)
        ->setPosition(20, height/2 - iconSize/2)
        ->setTouchable(true)
        ->setAction(
            [webView]() {
                printf("Got back\n");
                return true;
            }
        )
    );

    child(
        (new ImageElement(RAMFS "res/icons/forward.svg"))
        ->setSize(iconSize, iconSize)
        ->setPosition(70, height/2 - iconSize/2)
        ->setTouchable(true)
        ->setAction(
            [webView]() {
                printf("Got forward\n");
                return true;
            }
        )
    );

    child(
        (new ImageElement(RAMFS "res/icons/add.svg"))
        ->setSize(iconSize, iconSize)
        ->setPosition(width - 110, height/2 - iconSize/2)
        ->setTouchable(true)
        ->setAction(
            [webView]() {
                printf("Got Add\n");
                return true;
            }
        )
    );

    child(
        (new ImageElement(RAMFS "res/icons/tabs.svg"))
        ->setSize(iconSize, iconSize)
        ->setPosition(width - 60, height/2 - iconSize/2)
        ->setTouchable(true)
        ->setAction(
            [webView]() {
                printf("Got Tabs\n");
                return true;
            }
        )
    );
    
    
    // auto backIcon = new ImageElement("assets/back.png");
    // backIcon->constrain(ALIGN_CENTER_VERTICAL, 0);
    // child(b)
}

void URLBar::updateInfo() {
    urlText->setText(webView->url);
    urlText->update();
}

void URLBar::showKeyboard() {
    auto keyboard = new EKeyboard();
    keyboard->hasRoundedKeys = true;
    keyboard->updateSize();
    child(keyboard);
}

bool URLBar::process(InputEvents* event) {
    if (event->isTouchDown()) {
        auto innerWidth = width * 0.8;
        auto innerHeight = height * 0.6;
        if (event->touchIn(
            width/2 -  innerWidth/2,
            height/2 - innerHeight/2,
            innerWidth,
            innerHeight
        )) {
            showKeyboard();
            return true;
        }
    }
    return Element::process(event);
}

void URLBar::render(Element* parent) {
    // Draw background

    CST_Rect rect = { 0, 0, width, height };
    CST_SetDrawColorRGBA(RootDisplay::renderer, 0xdd, 0xdd, 0xdd, 0xff);
    CST_FillRect(RootDisplay::renderer, &rect);

    auto innerWidth = width * 0.8;
    auto innerHeight = height * 0.6;

    CST_roundedBoxRGBA(
        RootDisplay::renderer,
        width/2 -  innerWidth/2,
        height/2 - innerHeight/2,
        width/2 +  innerWidth/2,
        height/2 + innerHeight/2,
        10, 0xee, 0xee, 0xee, 0xff);

    Element::render(parent);
}