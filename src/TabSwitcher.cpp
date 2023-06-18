#include <sstream>
#include <iomanip>
#include "TabSwitcher.hpp"
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

    this->createTabCards();
}

void TabSwitcher::createTabCards() {
    wipeAll(false);

    // add close button in the top right
    child((new ImageElement(RAMFS "res/icons/x.svg"))
        ->setSize(35, 50)
        ->constrain(ALIGN_RIGHT, 30)
        ->constrain(ALIGN_TOP, 20)
        ->setAction([this]() {
            ((MainDisplay*)RootDisplay::mainDisplay)->switchSubscreen(NULL);
        })
        ->setAbsolute(true)
        ->setTouchable(true)
    );

    // TODO: use an actual Grid instead of a Container of Container rows
    Container* currentCon = NULL;

    auto allTabs = ((MainDisplay*)RootDisplay::mainDisplay)->allTabs;
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
        card->add((new ImageElement(tabPath.c_str()))->setSize(275, 155));

        currentCon->add(card);
        count++;
    }
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