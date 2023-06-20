#include <iostream>
#include "MainDisplay.hpp"
#include "WebView.hpp"
#include "URLBar.hpp"
#include <sys/stat.h>

MainDisplay::MainDisplay()
{
    // setScreenResolution(1920, 1080);

	RootDisplay::super();
    // backgroundColor = fromRGB(0x3c, 0xb0, 0x43);
	backgroundColor = fromRGB(0xff, 0xff, 0xff);

    srand(time(NULL));

    WebView* webView = new WebView();
	urlBar = new URLBar(webView);

    // TODO: load serialized tabs from disk
    allTabs.push_back(webView);

    // create some directory structures for favorites, history, and cache
    mkdir("./data", 0777);
    mkdir("./data/favorites", 0777);
    mkdir("./data/tabs", 0777);
    mkdir("./data/cache", 0777);

    favorites.push_back("https://vgmoose.com");
    favorites.push_back("https://www.wikipedia.org");
    favorites.push_back("https://www.google.com/webhp");
    favorites.push_back("https://serebii.net");
    favorites.push_back("https://en.wikipedia.org/wiki/Main_Page");
    favorites.push_back("https://en.wikipedia.org/wiki/Antarctica_(novel)");
    
}

bool MainDisplay::process(InputEvents* event)
{
    if (RootDisplay::subscreen)
		return RootDisplay::subscreen->process(event);

    bool ret = Element::process(event);
    if (ret) return true;

    if (event->quitaction == nullptr) {
        // set up quit callback to serialize the tabs
        event->quitaction = [this]() {
            auto summary = fullSessionSummary();
            printf("Saving session summary: %s\n", summary.c_str());
            FILE* f = fopen("./data/user.json", "w");
            fprintf(f, "%s", summary.c_str());
            fclose(f);
            exit(0);
        };
    }
    
	// keep processing child elements
    this->canUseSelectToExit = true;

    // manually process events for active tab and url bar

    // if we get a url bar press, don't continue
    if (urlBar->process(event)) {
        return true;
    }

    // if the active tab is interacted with, don't continue
    WebView* activeTab = getActiveWebView();
    if (activeTab != nullptr && activeTab->process(event)) {
        return true;
    }

	return false;
}

void MainDisplay::render(Element* parent)
{
    if (RootDisplay::subscreen)
	{
		RootDisplay::subscreen->render(this);
		this->update();
		return;
	}

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

std::string MainDisplay::fullSessionSummary() {
    std::string ret = "{\n";

    ret += "\t\"views\": [\n";

    // summarize each tab (and its history)
    for (int i = 0; i < allTabs.size(); i++) {
        WebView* webView = allTabs[i];
        ret += webView->fullSessionSummary();
        ret += ",\n";
    }

    if (favorites.size() > 0) {
        // delete two characters (trailing newline and last comma)
        ret = ret.substr(0, ret.size() - 2);
        ret += "\n\t],\n";
        
        ret += "\t\"favorites\": [\n";
        for (int i = 0; i < favorites.size(); i++) {
            ret += "\t\t\"" + favorites[i] + "\",\n";
        }
    }

    // delete two characters (trailing newline and last comma)
    ret = ret.substr(0, ret.size() - 2);

    ret += "\n\t]\n";
    ret += "}\n";

    return ret;
}