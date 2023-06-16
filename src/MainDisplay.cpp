#include "MainDisplay.hpp"
#include "WebView.hpp"
#include "URLBar.hpp"

MainDisplay::MainDisplay()
{
	RootDisplay::super();
    // backgroundColor = fromRGB(0x3c, 0xb0, 0x43);
	backgroundColor = fromRGB(0xff, 0xff, 0xff);

    WebView* webView = new WebView();
	urlBar = new URLBar(webView);

    // TODO: load serialized tabs from disk
    allTabs.push_back(webView);
}

bool MainDisplay::process(InputEvents* event)
{
	// keep processing child elements
    this->canUseSelectToExit = true;

    // manually process events for active tab and url bar

    // if we get a url bar press, don't continue
    if (urlBar->process(event)) {
        printf("Got url bar press\n");
        return true;
    }

    // if the active tab is interacted with, don't continue
    WebView* activeTab = getActiveWebView();
    if (activeTab != nullptr && activeTab->process(event)) {
        return true;
    }

	return RootDisplay::process(event);
}

void MainDisplay::render(Element* parent)
{
    // render the rest of the subelements (not RootDisplay's render)
	Element::render(parent);

    // we don't actually have children elements, manually render the active tab and the url bar
    WebView* activeTab = getActiveWebView();
    if (activeTab != nullptr) {
        activeTab->render(this);
    }
    urlBar->render(this);

	// commit everything to the screen manually (usually RootDisplay does this)
	this->update();
}

WebView* MainDisplay::setActiveTab(int index)
{
    // hide the currently active tab
    // WebView* activeTab = allTabs[activeTabIndex];

    // show the new tab
    activeTabIndex = index;
    WebView* newTab = getActiveWebView();
    return newTab;
}

// returns the new tab index
int MainDisplay::createNewTab() {
    // create a new webview, append it to all tabs
    WebView* newTab = new WebView();
    allTabs.push_back(newTab);
    return allTabs.size() - 1;
}

WebView* MainDisplay::getActiveWebView() {
    if (activeTabIndex >= allTabs.size()) {
        return allTabs[0]; // error handler, should always be here
    }
    return allTabs[activeTabIndex];
}

int MainDisplay::mainLoop()
{
	// for when we resize the main window, adjust some sizes
    RootDisplay::mainDisplay->windowResizeCallback = [this]() {
        auto webView = this->getActiveWebView();
        webView->width = RootDisplay::screenWidth;
        webView->height = RootDisplay::screenHeight;
        webView->needsRender = true;
        webView->needsRedraw = true;

        urlBar->width = RootDisplay::screenWidth;
        urlBar->recalcPosition(this);

		urlBar->futureRedrawCounter = 10; // redraw the URL bar for a few frames

        // TODO: put all images off-screen so that they won't be in the wrong place temporarily
    };

	int ret = RootDisplay::mainLoop();

	return ret;
}