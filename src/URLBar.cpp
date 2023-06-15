#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/Constraint.hpp"
#include "../libs/chesto/src/EKeyboard.hpp"

#define BUTTON_SIZE 50

Element* makeURLBarButton(const char* iconPath, std::function<bool()> action) {
    int iconSize = 30;
    Element* base = (new Element())
        ->setTouchable(true)
        ->setAction(action);

    base->width = BUTTON_SIZE;
    base->height = BUTTON_SIZE;

    base->child(
        (new ImageElement(iconPath))
        ->setSize(iconSize, iconSize)
        ->centerIn(base)
    );

    // center the button in the middle of the bar
    base->constrain(ALIGN_CENTER_VERTICAL, 0);

    return base;
}

URLBar::URLBar(WebView* webView)
{
    this->webView = webView;

    this->width = RootDisplay::screenWidth;
    this->height = 75;

    this->isAbsolute = true;

    CST_Color gray = { 80, 80, 80, 0xff };

    urlText = new TextElement(webView->url.c_str(), 28, &gray);
    urlText->constrain(ALIGN_CENTER_HORIZONTAL, 0);
    urlText->constrain(ALIGN_CENTER_VERTICAL, 0);
    child(urlText);

    auto innerWidth = width * 0.8; // TODO: make a variable somewhere
    int btnAndPadding = BUTTON_SIZE + 10;
    int sidePadding = 14;

    child(makeURLBarButton(RAMFS "res/icons/back.svg", [webView]() {
        printf("Got back\n");
        return true;
    })->constrain(ALIGN_LEFT, sidePadding));
    // })->setPosition(width/2 - innerWidth/2 - btnAndPadding * 2 + 5, 0));

    forwardButton = makeURLBarButton(RAMFS "res/icons/forward.svg", [webView]() {
        printf("Got forward\n");
        return true;
    })->constrain(ALIGN_LEFT, sidePadding/2 + btnAndPadding);
    // })->setPosition(width/2 - innerWidth/2 - btnAndPadding - 5, 0);
    // child(forwardButton);

    clockButton = (new ClockElement())
        ->constrain(ALIGN_CENTER_VERTICAL, 0)
        ->constrain(ALIGN_LEFT, sidePadding/2 + btnAndPadding);
        // ->setPosition(forwardButton->x, 0);
    child(clockButton);

    child(makeURLBarButton(RAMFS "res/icons/add.svg", [webView]() {
        printf("Got Add\n");
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding/2 + btnAndPadding));
    // })->setPosition(width/2 + innerWidth/2 + 9, 0));

    child(makeURLBarButton(RAMFS "res/icons/tabs.svg", [webView]() {
        printf("Got Tabs\n");
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding));
    // })->setPosition(width/2 + innerWidth/2 + btnAndPadding + 9 - 5, 0));
    
    
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
    auto innerWidth = width * 0.8;
    auto innerHeight = height * 0.75;

    if (event->touchIn(
        width/2 -  innerWidth/2,
        height/2 - innerHeight/2,
        innerWidth,
        innerHeight
    )) {
        if (event->isTouchDown()) {
            highlightingKeyboard = true;
            return true;
        } else if (highlightingKeyboard && event->isTouchUp()) {
            showKeyboard();
            highlightingKeyboard = false;
            return true;
        }
    } else if (event->isTouchDrag()) {
        // we're dragging outside of the url bar
        highlightingKeyboard = false;
        return true;
    }

    return Element::process(event);
}

void URLBar::render(Element* parent) {
    // Draw background

    CST_Rect rect = { 0, 0, width, height };
    CST_SetDrawColorRGBA(RootDisplay::renderer, 0xdd, 0xdd, 0xdd, 0xff);
    CST_FillRect(RootDisplay::renderer, &rect);

    auto innerWidth = width * 0.8;
    auto innerHeight = height * 0.75;

    CST_roundedBoxRGBA(
        RootDisplay::renderer,
        width/2 -  innerWidth/2,
        height/2 - innerHeight/2,
        width/2 +  innerWidth/2,
        height/2 + innerHeight/2,
        15, 0xee, 0xee, 0xee, 0xff);
    
    if (highlightingKeyboard) {
        CST_roundedBoxRGBA(
            RootDisplay::renderer,
            width/2 -  innerWidth/2,
            height/2 - innerHeight/2,
            width/2 +  innerWidth/2,
            height/2 + innerHeight/2,
            15, 0xad, 0xd8, 0xe6, 0x90);
        CST_roundedRectangleRGBA(
            RootDisplay::renderer,
            width/2 -  innerWidth/2,
            height/2 - innerHeight/2,
            width/2 +  innerWidth/2,
            height/2 + innerHeight/2,
            15, 0x66, 0x7c, 0x89, 0x90);
        
    }

    Element::render(parent);
}