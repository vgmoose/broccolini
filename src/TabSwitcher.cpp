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

    hasBackground = true;
    backgroundColor = { 1, 1, 1 };

    // this->createTabCards();
}


// Chesto's switchSubscreen will try to free the subscreen, which we don't want,
// so instead we'll set subscreen manually (and the MainDisplay's process method accounts for this)
void TabSwitcher::createTabCards() {
    wipeAll(false);

    auto mainDisplay = ((MainDisplay*)RootDisplay::mainDisplay);

    // add close button in the top right
    child((new ImageElement(RAMFS "res/icons/x.svg"))
        ->setSize(35, 50)
        ->constrain(ALIGN_RIGHT, 30)
        ->constrain(ALIGN_TOP, 20)
        ->setAction([this, mainDisplay]() {
            mainDisplay->subscreen = NULL;
        })
        ->setAbsolute(true)
        ->setTouchable(true)
    );

    // TODO: use an actual Grid instead of a Container of Container rows
    Container* currentCon = NULL;

    auto allTabs = mainDisplay->allTabs;
    auto black = CST_MakeColor(0, 0, 0, 255);

    int nextConYPos = -70;

    // go through all tabs and make a card for each one
    int count = 0;
    for (auto tab : allTabs) {
        if (currentCon == NULL || count % 4 == 0) {
            currentCon = new Container(ROW_LAYOUT, 20);
            currentCon->constrain(ALIGN_CENTER_HORIZONTAL);
            child(currentCon);
            nextConYPos += 110;
            currentCon->setPosition(0, nextConYPos);
        }
        auto tabPath = "./data/tabs/" + tab->id + ".png";
        auto tabName = just_domain_from_url(tab->url.c_str());

        auto card = new Container(COL_LAYOUT, 8);
        card->add(new TextElement(tabName.c_str(), 20, &black));

        auto thumbnail = new ImageElement(tabPath.c_str());
        thumbnail->loadPath(tabPath, true); // force bypass cache, to get fresh thumbnails
        card->add(thumbnail->setSize(275, 155));

        card->setAction([this, count, mainDisplay]() {
            mainDisplay->setActiveTab(count);
            mainDisplay->subscreen = NULL;

            // update the URLBar's current tab
            auto urlBar = mainDisplay->urlBar;
            urlBar->webView = mainDisplay->getActiveWebView();
        });
        card->setTouchable(true);

        currentCon->add(card);
        count++;
    }

    futureRedrawCounter = 2;
}

bool TabSwitcher::process(InputEvents* event) {
    return ListElement::process(event);
}

void TabSwitcher::render(Element* parent) {
    if (hidden) return;

    // draw a white bg (TODO: use chesto methods / debug why hasBackground wasn't enough)
    SDL_SetRenderDrawColor(RootDisplay::renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(RootDisplay::renderer, NULL);

    ListElement::render(parent);
}