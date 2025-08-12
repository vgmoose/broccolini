#include "TabSwitcher.hpp"
#include "../libs/chesto/src/Constraint.hpp"
#include "../libs/chesto/src/Container.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../utils/UIUtils.hpp"
#include "../utils/Utils.hpp"
#include "./MainDisplay.hpp"
#include "URLBar.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

TabSwitcher::TabSwitcher()
{
	this->width = RootDisplay::screenWidth;
	this->height = RootDisplay::screenHeight;

	// hasBackground = true;
	// backgroundColor = { 1, 1, 1 };

	// this->createTabCards();
}

// Chesto's switchSubscreen will try to free the subscreen, which we don't want,
// so instead we'll set subscreen manually (and the MainDisplay's process method
// accounts for this)
void TabSwitcher::createTabCards()
{
	wipeAll(false);

	auto mainDisplay = ((MainDisplay*)RootDisplay::mainDisplay);
	auto urlBar = mainDisplay->urlBar;

	// TODO: use an actual Grid instead of a Container of Container rows
	Container* currentCon = NULL;

	std::vector<WebView*> curTabs = *mainDisplay->getAllTabs();
	curTabs.push_back(NULL); // the last tab is the new tab button

	auto black = CST_MakeColor(0, 0, 0, 255);
	if (mainDisplay->privateMode)
	{
		black = CST_MakeColor(255, 255, 255, 255); // rename var?
	}

	int nextConYPos = -70;
	ImageElement* activeTabOverlay = NULL;

	auto currentTabPath = urlBar->getCurTabScreenshotPath();
	Element* lastActiveThumbnail = NULL;

	// go through all tabs and make a card for each one
	int count = 0;
	int cardCount = 4; // RootDisplay::screenWidth / 275;
	for (auto tab : curTabs)
	{
		if (currentCon == NULL || count % cardCount == 0)
		{
			currentCon = new Container(ROW_LAYOUT, 20);
			currentCon->constrain(ALIGN_CENTER_HORIZONTAL);
			child(currentCon);
			nextConYPos += 110;
			currentCon->setPosition(0, nextConYPos);
		}
		if (tab == NULL)
		{
			// new tab card for the last element
			auto newTabCard = new Container(COL_LAYOUT, 8);
			auto placeHolder = mainDisplay->privateMode ? "New Private Tab" : "New Tab";
			auto textElem = new TextElement(placeHolder, 20, &black);
			newTabCard->add(textElem);
			textElem->update(true);
			auto cardImgPath = RAMFS "res/newtab.png";
			auto newCardImage = (new ImageElement(cardImgPath))->setSize(275, 155);
			newTabCard->add(newCardImage);
			newTabCard->setAction(
				[this, mainDisplay, urlBar, cardImgPath, newCardImage]()
				{
					auto tabId = mainDisplay->createNewTab();
					mainDisplay->setActiveTab(tabId);

					// update the URLBar's current tab
					urlBar->webView = mainDisplay->getActiveWebView();
					urlBar->currentUrl = urlBar->webView->url;
					urlBar->updateInfo();

					this->close(cardImgPath, newCardImage);
				});
			newTabCard->setTouchable(true);
			currentCon->add(newTabCard);
			count++; // shouldn't matter

			if (lastActiveThumbnail == NULL)
			{
				// if we don't have a last active thumbnail, use the new tab card
				lastActiveThumbnail = newCardImage;
			}
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
		thumbnail->loadPath(tabPath,
			true); // force bypass cache, to get fresh thumbnails

		card->add(thumbnail->setSize(275, 155));

		card->setAction([this, count, mainDisplay, urlBar, card, thumbnail]()
			{
      mainDisplay->setActiveTab(count);

      // update the URLBar's current tab
      urlBar->webView = mainDisplay->getActiveWebView();
      urlBar->currentUrl = urlBar->webView->url;
      urlBar->updateInfo();

      this->close(urlBar->getCurTabScreenshotPath(), thumbnail); });
		card->setTouchable(true);

		currentCon->add(card);
		count++;

		// if this card is the current tab, animate an overlay to its position
		if (tabPath == currentTabPath)
		{
			// save the active thumbnail for potential animating later
			lastActiveThumbnail = thumbnail;

			activeTabOverlay = new ImageElement(tabPath);
			activeTabOverlay->isAbsolute = true;
			activeTabOverlay->loadPath(tabPath, true);

			int startX = 0;
			int startY = urlBar->height;
			int endX = this->x + currentCon->x + card->x + thumbnail->x;
			int endY = this->y + currentCon->y + card->y + thumbnail->y;

			int startWidth = activeTabOverlay->width;
			int startHeight = activeTabOverlay->height;

			activeTabOverlay->setPosition(startX, startY);

			// the current tab starts full screen, and animates to the card size
			activeTabOverlay->animate(
				200,
				[startX, startY, this, currentCon, card, thumbnail, startWidth,
					startHeight, activeTabOverlay](float progress)
				{
					// TODO: these end/start positions aren't correct after scrolling
					int endX = thumbnail->xAbs;
					int endY = thumbnail->yAbs;

					int curX = startX * (1.0 - progress) + endX * progress;
					int curY = startY * (1.0 - progress) + endY * progress;
					activeTabOverlay->setPosition(curX, curY);

					int curWidth = 275 * progress + startWidth * (1.0 - progress);
					int curHeight = 155 * progress + startHeight * (1.0 - progress);
					activeTabOverlay->setSize(curWidth, curHeight);
				},
				[this, activeTabOverlay]()
				{
					// remove the overlay when the animation is done
					this->remove(activeTabOverlay);
					this->needsRedraw = true;
				});
		}
	}

	futureRedrawCounter = 10; // redraw for a few frames

	// add close button in the top right
	closeButton = createThemedIcon("x")
					  ->setSize(35, 50)
					  ->constrain(ALIGN_RIGHT, 22)
					  ->setAction([this, mainDisplay, currentTabPath,
									  lastActiveThumbnail]()
						  {
                      this->close(currentTabPath, lastActiveThumbnail);
                      // if our current tab list is empty, create a new tab
                      if (mainDisplay->getAllTabs()->size() == 0) {
                        mainDisplay->goToStartPage();
                      } })
					  ->setAbsolute(true)
					  ->setTouchable(true);
	closeButton->y = 12;
	child(closeButton);

	privateButton = createThemedIcon("private")
						->setSize(40, 45)
						->constrain(ALIGN_LEFT, 22)
						->setAction([this, mainDisplay, urlBar]()
							{
                        mainDisplay->privateMode = !mainDisplay->privateMode;
                        this->createTabCards();
                        urlBar->updateInfo(); })
						->setAbsolute(true)
						->setTouchable(true);
	privateButton->y = 12;
	child(privateButton);

	// start a slide up animation for the urlbar
	urlBar->animate(
		200,
		[urlBar](float progress)
		{ urlBar->y = 0 - urlBar->height * progress; },
		[urlBar]()
		{
			urlBar->y = 0 - urlBar->height; // fully offscreen
		});

	if (activeTabOverlay != NULL)
	{
		this->child(activeTabOverlay);
	}
}

void TabSwitcher::close(std::string imgPath, Element* thumbnail)
{
	auto mainDisplay = ((MainDisplay*)RootDisplay::mainDisplay);
	auto urlBar = mainDisplay->urlBar;

	// clear all url bar animations
	urlBar->animations.clear();

	// start a slide down animation for the urlbar
	urlBar->animate(
		200,
		[urlBar](float progress)
		{
			urlBar->y = 0 - urlBar->height * (1.0 - progress);
		},
		[urlBar]()
		{ urlBar->y = 0; });

	// create an overlay over the current tab to animate full screen
	auto overlay = new ImageElement(imgPath);
	overlay->isAbsolute = true;
	overlay->loadPath(imgPath, true);
	overlay->setSize(thumbnail->width, thumbnail->height);
	overlay->setPosition(thumbnail->xAbs, thumbnail->yAbs);
	this->child(overlay);

	// animate the overlay to full screen
	overlay->animate(
		200,
		[overlay, thumbnail, urlBar, this](float progress)
		{
			int startX = thumbnail->xAbs;
			int startY = thumbnail->yAbs;
			int endX = 0;
			int endY = urlBar->height;
			int curX = endX * progress + startX * (1.0 - progress);
			int curY = endY * progress + startY * (1.0 - progress);
			overlay->setPosition(curX, curY);

			int startWidth = thumbnail->width;
			int startHeight = thumbnail->height;
			int curWidth = RootDisplay::screenWidth * progress + startWidth * (1.0 - progress);
			int curHeight = RootDisplay::screenHeight * progress + startHeight * (1.0 - progress);
			overlay->setSize(curWidth, curHeight);
		},
		[overlay, mainDisplay, this]()
		{
			this->remove(overlay);
			this->needsRedraw = true;
			// actually close the subscreen
			mainDisplay->subscreen = NULL;
		});
}

bool TabSwitcher::process(InputEvents* event)
{
	return ListElement::process(event);
}

void TabSwitcher::render(Element* parent)
{
	if (hidden)
		return;

	auto mainDisplay = ((MainDisplay*)RootDisplay::mainDisplay);

	// draw a white bg (TODO: use chesto methods / debug why hasBackground wasn't
	// enough)
	CST_SetDrawColorRGBA(RootDisplay::renderer, 255, 255, 255, 255);
	if (mainDisplay->privateMode)
		CST_SetDrawColorRGBA(RootDisplay::renderer, 0x66, 0x66, 0x66, 255);
	CST_FillRect(RootDisplay::renderer, NULL);

	ListElement::render(parent);

	// draw a circle around close button
	auto closeRect = closeButton->getBounds();
	CST_filledCircleRGBA(RootDisplay::renderer, closeRect.x + closeRect.w * 1.5,
		closeRect.y, closeRect.w * 2.5, 0xee, 0xee, 0xee, 255);

	// draw a circle around private button
	auto privateRect = privateButton->getBounds();
	CST_filledCircleRGBA(RootDisplay::renderer, privateRect.x - privateRect.w / 2,
		closeRect.y, closeRect.w * 2.5, 0xee, 0xee, 0xee, 255);

	// double render the close button, so it's on top of the tab cards
	// TODO: some kind of z-index system or drawing priority thing
	closeButton->render(this);
	privateButton->render(this);

	// draw the url bar at the very end, from under the subscreen, which will
	// animate out
	mainDisplay->urlBar->render(this);
}