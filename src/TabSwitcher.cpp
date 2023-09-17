#include <sstream>
#include <iomanip>
#include "TabSwitcher.hpp"
#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/Container.hpp"
#include "../libs/chesto/src/Constraint.hpp"
#include "./MainDisplay.hpp"

TabSwitcher::TabSwitcher()
{
    this->width = RootDisplay::screenWidth;
    this->height = RootDisplay::screenHeight;

    // hasBackground = true;
    // backgroundColor = { 1, 1, 1 };

    // this->createTabCards();
}


// Chesto's switchSubscreen will try to free the subscreen, which we don't want,
// so instead we'll set subscreen manually (and the MainDisplay's process method accounts for this)
void TabSwitcher::createTabCards() {
    wipeAll(false);

    auto mainDisplay = ((MainDisplay*)RootDisplay::mainDisplay);

    // add close button in the top right
    closeButton = (new ImageElement(RAMFS "res/icons/x.svg"))
        ->setSize(35, 50)
        ->constrain(ALIGN_RIGHT, 22)
        ->setAction([this, mainDisplay]() {
            mainDisplay->subscreen = NULL;
            // if our current tab list is empty, create a new tab
            if (mainDisplay->getAllTabs()->size() == 0) {
                mainDisplay->goToStartPage();
            }
        })
        ->setAbsolute(true)
        ->setTouchable(true);
    closeButton->y = 12;
    child(closeButton);

    privateButton = (new ImageElement(RAMFS "res/icons/private.svg"))
        ->setSize(40, 45)
        ->constrain(ALIGN_LEFT, 22)
        ->setAction([this, mainDisplay]() {
            mainDisplay->privateMode = !mainDisplay->privateMode;
            auto urlBar = mainDisplay->urlBar;
            this->createTabCards();
            urlBar->updateInfo();
        })
        ->setAbsolute(true)
        ->setTouchable(true);
    privateButton->y = 12;
    child(privateButton);

    // TODO: use an actual Grid instead of a Container of Container rows
    Container* currentCon = NULL;

    std::vector<WebView*> curTabs = *mainDisplay->getAllTabs();
    curTabs.push_back(NULL); // the last tab is the new tab button

    auto black = CST_MakeColor(0, 0, 0, 255);
    if (mainDisplay->privateMode) {
        black = CST_MakeColor(255, 255, 255, 255); // rename var?
    }

    int nextConYPos = -70;

    // go through all tabs and make a card for each one
    int count = 0;
    int cardCount = 4; //RootDisplay::screenWidth / 275;
    for (auto tab : curTabs) {
        if (currentCon == NULL || count % cardCount == 0) {
            currentCon = new Container(ROW_LAYOUT, 20);
            currentCon->constrain(ALIGN_CENTER_HORIZONTAL);
            child(currentCon);
            nextConYPos += 110;
            currentCon->setPosition(0, nextConYPos);
        }
        if (tab == NULL) {
            // new tab card for the last element
            auto newTabCard = new Container(COL_LAYOUT, 8);
            auto placeHolder = mainDisplay->privateMode ? "New Private Tab" : "New Tab";
            auto textElem = new TextElement(placeHolder, 20, &black);
            newTabCard->add(textElem);
            textElem->update(true);
            newTabCard->add((new ImageElement(RAMFS "res/newtab.png"))->setSize(275, 155));
            newTabCard->setAction([this, mainDisplay]() {
                auto tabId = mainDisplay->createNewTab();
                mainDisplay->setActiveTab(tabId);
                mainDisplay->subscreen = NULL;

                // update the URLBar's current tab
                auto urlBar = mainDisplay->urlBar;
                urlBar->webView = mainDisplay->getActiveWebView();
                urlBar->updateInfo();
            });
            newTabCard->setTouchable(true);
            currentCon->add(newTabCard);
            count++; // shouldn't matter
            continue;
        }
        std::string viewPath = mainDisplay->privateMode ? "pviews" : "views";
        auto tabPath = "./data/" + viewPath + "/" + tab->id + ".png";
        auto tabName = just_domain_from_url(tab->url.c_str());

        auto card = new Container(COL_LAYOUT, 8);
        auto textElem = new TextElement(tabName.c_str(), 20, &black);
        textElem->update(true);
        card->add(textElem);

        auto thumbnail = new ImageElement(tabPath.c_str());
        thumbnail->loadPath(tabPath, true); // force bypass cache, to get fresh thumbnails
        card->add(thumbnail->setSize(275, 155));

        card->setAction([this, count, mainDisplay]() {
            mainDisplay->setActiveTab(count);
            mainDisplay->subscreen = NULL;

            // update the URLBar's current tab
            auto urlBar = mainDisplay->urlBar;
            urlBar->webView = mainDisplay->getActiveWebView();
            urlBar->updateInfo();
        });
        card->setTouchable(true);

        currentCon->add(card);
        count++;
    }

    futureRedrawCounter = 10; // redraw for a few frames
}

bool TabSwitcher::process(InputEvents* event) {
    return ListElement::process(event);
}

void TabSwitcher::render(Element* parent) {
    if (hidden) return;

    auto mainDisplay = ((MainDisplay*)RootDisplay::mainDisplay);

    // draw a white bg (TODO: use chesto methods / debug why hasBackground wasn't enough)
    CST_SetDrawColorRGBA(RootDisplay::renderer, 255, 255, 255, 255);
    if (mainDisplay->privateMode)
        CST_SetDrawColorRGBA(RootDisplay::renderer, 0x66, 0x66, 0x66, 255);
    CST_FillRect(RootDisplay::renderer, NULL);

    ListElement::render(parent);

    // draw a circle around close button
    auto closeRect = closeButton->getBounds();
    CST_filledCircleRGBA(RootDisplay::renderer, closeRect.x + closeRect.w * 1.5, closeRect.y, closeRect.w * 2.5, 0xee, 0xee, 0xee, 255);

    // draw a circle around private button
    auto privateRect = privateButton->getBounds();
    CST_filledCircleRGBA(RootDisplay::renderer, privateRect.x - privateRect.w / 2, closeRect.y, closeRect.w * 2.5, 0xee, 0xee, 0xee, 255);

    // double render the close button, so it's on top of the tab cards
    // TODO: some kind of z-index system or drawing priority thing
    closeButton->render(this);
    privateButton->render(this);
}