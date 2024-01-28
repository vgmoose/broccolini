#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/Constraint.hpp"
#include "./MainDisplay.hpp"
#include "TabSwitcher.hpp"
#include "../utils/UIUtils.hpp"

#define BUTTON_SIZE 50

void printHistory(WebView* webView) {
    printf("=========\nHistory:\n");
    for (int i = 0; i < webView->history.size(); i++) {
        printf("%c%d: %s\n", i == webView->historyIndex ? '*' : ' ', i, webView->history[i].c_str());
    }
    printf("=========\n");
}

Element* URLBar::makeURLBarButton(std::string iconPath, std::function<bool()> action) {
    int iconSize = 30;
    Element* base = (new Element())
        ->setTouchable(true)
        ->setAction(action);

    base->width = BUTTON_SIZE;
    base->height = BUTTON_SIZE;

    base->child(
        createThemedIcon(iconPath + (themeIsTooDark ? "-light" : ""))
        ->setSize(iconSize, iconSize)
        ->centerIn(base)
    );

    // center the button in the middle of the bar
    base->constrain(ALIGN_CENTER_VERTICAL, 0);

    return base;
}

void URLBar::resetBar() {
    // update the webview that the urlbar points to
    this->webView = ((MainDisplay*)RootDisplay::mainDisplay)->getActiveWebView();

    // reload the url text, bring to the front, and update the clock/forward state
    currentUrl = webView->url.c_str();
    this->updateVisibleUrlText();

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

    createURLBarElements();
}

void URLBar::createURLBarElements() {
    removeAll(true);
    keyboard = NULL;

    auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;

    CST_Color gray = { 80, 80, 80, 0xff };

    urlText = new TextElement(webView->url.c_str(), 28, &gray);
    urlText->constrain(ALIGN_CENTER_HORIZONTAL, 0);
    urlText->constrain(ALIGN_CENTER_VERTICAL, 0);
    child(urlText);
    this->updateVisibleUrlText();

    auto innerWidth = width * 0.8; // TODO: make a variable somewhere
    int btnAndPadding = BUTTON_SIZE + 10;
    int sidePadding = 14;

    child(makeURLBarButton("back", [this]() {
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

    forwardButton = makeURLBarButton("forward", [this]() {
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

    bool use12HourClock = true;
    if (clockButton != NULL) {
        use12HourClock = ((ClockElement*)clockButton)->is12Hour;
    }

    clockButton = (new ClockElement(themeIsTooDark, use12HourClock))
        ->constrain(ALIGN_CENTER_VERTICAL, 0)
        ->constrain(ALIGN_LEFT, sidePadding/3 + btnAndPadding);
    child(clockButton);

    child(makeURLBarButton("add", [this, mainDisplay]() {
        // create a new webview
        saveCurTabScreenshot(mainDisplay->privateMode);
        int idx = mainDisplay->createNewTab();
        this->webView = mainDisplay->setActiveTab(idx);
        this->resetBar();
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding/3 + btnAndPadding + 1));

    child(makeURLBarButton("tabs", [this, mainDisplay]() {
        saveCurTabScreenshot(mainDisplay->privateMode);

        // set up the new tab screen 
        if (tabSwitcher == NULL) {
            tabSwitcher = new TabSwitcher();
        }
        tabSwitcher->createTabCards();
        mainDisplay->subscreen = tabSwitcher;
        this->resetBar();
        
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding));
    
    
    // add bookmark and refresh buttons within the url text
    // TODO: actually put these in the bar, instead of just being normal buttons
    bookmarkButton = makeURLBarButton("star", [this, mainDisplay]() {
        auto webView = this->webView;
        // take a screen shot of the current view
        auto favThumbPath = "./data/favorites/" + base64_encode(webView->url) + ".png";
        webView->screenshot(favThumbPath);
        // add the url to the favorites list
        mainDisplay->favorites.push_back(webView->url);
        return true;
    })->constrain(ALIGN_LEFT, sidePadding/3 + btnAndPadding*2 + 10);
    child(bookmarkButton);

    privateIndicator = makeURLBarButton("private", [this, mainDisplay]() {
        // exit the private mode, and make a new tab
        // we need a screen shot before leaving
        saveCurTabScreenshot(true);
        mainDisplay->privateMode = false;
        mainDisplay->goToStartPage();
        this->updateInfo();
        this->resetBar();
        return true;
    })->constrain(ALIGN_LEFT, sidePadding/3 + btnAndPadding*2 + 10);
    privateIndicator->hidden = true;
    child(privateIndicator);

    child(makeURLBarButton("refresh", [this]() {
        auto webView = this->webView;
        webView->needsLoad = true;
        return true;
    })->constrain(ALIGN_RIGHT, sidePadding/3 + btnAndPadding*2 + 10));
}

void URLBar::updateInfo() {
    auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;

    privateIndicator->hidden = true;
    bookmarkButton->hidden = false;
    CST_Color color = { 80, 80, 80, 0xff };

    if (mainDisplay->privateMode) {
        privateIndicator->hidden = false;
        bookmarkButton->hidden = true;
        color = { 255, 255, 255, 0xff };
    }

    getThemeColor();

    if (themeIsTooDark) {
        color = { 255, 255, 255, 0xff };
    }

    urlText->setColor(color);
    this->updateVisibleUrlText();
}

void URLBar::showKeyboard() {

    // TODO: allow text selection and editing, not just wipe
    this->currentUrl = "";
    this->updateVisibleUrlText();

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
            if (this->currentUrl.length() <= 0) return;
            this->currentUrl = this->currentUrl.substr(0, this->currentUrl.length() - 1);
            this->updateInfo();
            return;
        }
        if (text == '\n') {
            // enter
            this->webView->url = this->currentUrl;
            this->webView->needsLoad = true;
            this->keyboard->hidden = true;
            return;
        }
        this->currentUrl += text;
        this->updateVisibleUrlText();
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

CST_Color URLBar::getThemeColor() {
    CST_Color theme_color = { 0xdd, 0xdd, 0xdd, 0xff };

    bool prevThemeColor = themeIsTooDark;

    // get the theme color from the active webview, and determine if it's too dark
    themeIsTooDark = false;
    if (webView != NULL) {
        theme_color = webView->theme_color;

        if (theme_color.r + theme_color.g + theme_color.b < 0x66 * 3) {
            // too dark, let's use white text
            themeIsTooDark = true;
        }
    }

    if (prevThemeColor != themeIsTooDark) {
        // when theme changes, recreate the bar elements
        createURLBarElements();
    }

    return theme_color;
}

void URLBar::render(Element* parent) {
    // Draw background
    auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
    
    CST_Rect rect = { x, y, x + width, y + height };

    auto theme_color = getThemeColor();

    CST_SetDrawColorRGBA(RootDisplay::renderer, theme_color.r, theme_color.g, theme_color.b, 0xff);
    CST_FillRect(RootDisplay::renderer, &rect);

    auto innerWidth = width * 0.8;
    auto innerHeight = height * 0.75;
    
    if (mainDisplay->privateMode) {
        auto darkInnerWidth = innerWidth * 0.85;
        CST_roundedBoxRGBA(
            RootDisplay::renderer,
            x + width/2 -  darkInnerWidth/2,
            y + height/2 - innerHeight/2,
            x + width/2 +  darkInnerWidth/2,
            y + height/2 + innerHeight/2,
            15, 0x66, 0x66, 0x66, 0xff);
    } else {
        CST_roundedBoxRGBA(
            RootDisplay::renderer,
            x + width/2 -  innerWidth/2,
            y + height/2 - innerHeight/2,
            x + width/2 +  innerWidth/2,
            y + height/2 + innerHeight/2, 15,
            fmin(theme_color.r + 0x11, 0xff),
            fmin(theme_color.g + 0x11, 0xff),
            fmin(theme_color.b + 0x11, 0xff), 0xff);
    }
    
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

void URLBar::updateVisibleUrlText() {
    // set the text to the current url
    this->urlText->setText(this->currentUrl);
    this->urlText->update(true);

    // if the text is too long, truncate it and add an ellipsis
    auto innerWidth = width * 0.8 - (BUTTON_SIZE * 2 + 10); // some padding
    auto urlTextWidth = this->urlText->width;
    int trimOffset = 0;
    while (urlTextWidth > innerWidth) {
        // keep ellipsis-ing the middle of the URL until it fits
        trimOffset += 2;
        this->urlText->setText(
            this->currentUrl.substr(0, this->currentUrl.length()/2 - trimOffset) +
            "…" +
            this->currentUrl.substr(this->currentUrl.length()/2 + trimOffset)
        );
        this->urlText->update(true);
        urlTextWidth = this->urlText->width;
    }

    // update the window title
    if (webView != NULL) {
        CST_SetWindowTitle(webView->windowTitle.c_str());
    }
}

std::string URLBar::getCurTabScreenshotPath() {
    auto webView = ((MainDisplay*)RootDisplay::mainDisplay)->getActiveWebView();
    std::string viewFolder = ((MainDisplay*)RootDisplay::mainDisplay)->privateMode ? "pviews" : "views";
    auto screenshotPath = "./data/" + viewFolder + "/" + webView->id + ".png";
    return screenshotPath;
}

void URLBar::saveCurTabScreenshot(bool isPrivate) {
    auto webView = ((MainDisplay*)RootDisplay::mainDisplay)->getActiveWebView();
    auto screenshotPath = getCurTabScreenshotPath();
    webView->screenshot(screenshotPath);
}