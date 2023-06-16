#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/Constraint.hpp"
#include "./MainDisplay.hpp"

#define BUTTON_SIZE 50

void printHistory(WebView* webView) {
    printf("=========\nHistory:\n");
    for (int i = 0; i < webView->history.size(); i++) {
        printf("%c%d: %s\n", i == webView->historyIndex ? '*' : ' ', i, webView->history[i].c_str());
    }
    printf("=========\n");
}

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

void URLBar::resetBar() {
    // reload the url text, bring to the front, and update the clock/forward state
    urlText->setText(webView->url.c_str());
    urlText->update();

    // if we're at the end of the history, show the clock
    if (webView->historyIndex >= webView->history.size() - 1) {
        forwardButton->hidden = true;
        clockButton->hidden   = false;
    } else {
        forwardButton->hidden = false;
        clockButton->hidden   = true;
    }
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

    child(makeURLBarButton(RAMFS "res/icons/back.svg", [this]() {
        auto webView = this->webView;
        webView->historyIndex --;
        if (webView->historyIndex < 0) {
            webView->historyIndex ++;
            return true;
        }
        webView->url = webView->history[webView->historyIndex];
        // printf("Going back to %s\n", webView->url.c_str());
        resetBar();
        webView->needsLoad = true;

        return true;
    })->constrain(ALIGN_LEFT, sidePadding - 1));

    forwardButton = makeURLBarButton(RAMFS "res/icons/forward.svg", [this]() {
        auto webView = this->webView;
        // printf("-------\n");
        // printHistory(webView);
        // set the next url
        webView->historyIndex ++;
        if (webView->historyIndex >= webView->history.size()) {
            webView->historyIndex --;
            return true;
        }
        webView->url = webView->history[webView->historyIndex];
        // printf("Going forward to %s\n", webView->url.c_str());
        resetBar();
        webView->needsLoad = true;
        return true;
    })->constrain(ALIGN_LEFT, sidePadding/3 + btnAndPadding);
    forwardButton->hidden = true;
    child(forwardButton);

    clockButton = (new ClockElement())
        ->constrain(ALIGN_CENTER_VERTICAL, 0)
        ->constrain(ALIGN_LEFT, sidePadding/3 + btnAndPadding);
    child(clockButton);

    child(makeURLBarButton(RAMFS "res/icons/add.svg", [this]() {
        // create a new webview
        auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
        int idx = mainDisplay->createNewTab();
        this->webView = mainDisplay->setActiveTab(idx);
        this->resetBar();
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding/3 + btnAndPadding + 1));

    child(makeURLBarButton(RAMFS "res/icons/tabs.svg", [this]() {
        // temporary: just go to the next tab
        auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
        this->webView = mainDisplay->setActiveTab((mainDisplay->activeTabIndex + 1) % mainDisplay->allTabs.size());
        this->resetBar();
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding));
    
    
    // add bookmark and refresh buttons within the url text
    // TODO: actually put these in the bar, instead of just being normal buttons
    child(makeURLBarButton(RAMFS "res/icons/star.svg", [webView]() {
        printf("Got Bookmark\n");
        return true;
    })->constrain(ALIGN_LEFT, sidePadding/3 + btnAndPadding*2 + 10));

    child(makeURLBarButton(RAMFS "res/icons/refresh.svg", [webView]() {
        webView->needsLoad = true;
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding/3 + btnAndPadding*2 + 10));

}

void URLBar::updateInfo() {
    urlText->setText(webView->url);
    urlText->update();
}

void URLBar::showKeyboard() {

    // TODO: allow text selection and editing, not just wipe
    this->urlText->setText("");
    this->urlText->update();

    if (keyboard != NULL) {
        // just show the existing keyboard
        keyboard->hidden = false;
        return;
    }

    keyboard = new EKeyboard();
    keyboard->hasRoundedKeys = true;
    keyboard->updateSize();
    child(keyboard);

    keyboard->typeAction = [this](char text) {
        if (text == '\b') {
            // backspace
            if (this->urlText->text.length() <= 0) return;
            this->urlText->setText(this->urlText->text.substr(0, this->urlText->text.length() - 1));
            this->urlText->update();
            return;
        }
        if (text == '\n') {
            // enter
            this->webView->url = this->urlText->text;
            this->webView->needsLoad = true;
            this->keyboard->hidden = true;
            return;
        }
        this->urlText->setText(this->urlText->text + text);
        this->urlText->update();
    };
}

bool URLBar::process(InputEvents* event) {
    auto innerWidth = width * 0.8;
    auto innerHeight = height * 0.75;

    if (Element::process(event)) {
        // one of our buttons was pushed, go with it
        return true;
    }
    
    if (event->touchIn(
        width/2 -  innerWidth/2,
        height/2 - innerHeight/2,
        innerWidth, innerHeight
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
        highlightingKeyboard = false;
        if (event->xPos < width && event->yPos < height) {
            // we're dragging outside of the text-editable area, but still on the URL bar, we can refresh
            return true;
        }
        return false;
    }

    return false;
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