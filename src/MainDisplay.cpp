#include <iostream>
#include "MainDisplay.hpp"
#include "WebView.hpp"
#include "URLBar.hpp"
#include "../utils/Utils.hpp"
#include <sys/stat.h>
#include <filesystem>

MainDisplay::MainDisplay()
{
    // setScreenResolution(1920, 1080);

	RootDisplay::super();
    // backgroundColor = fromRGB(0x3c, 0xb0, 0x43);
	backgroundColor = fromRGB(0xff, 0xff, 0xff);
    hasBackground = true;

    RootDisplay::idleCursorPulsing = true;

    srand(time(NULL));

    WebView* webView = new WebView();
	urlBar = new URLBar(webView);

    webView->y = urlBar->height;
    webView->minYScroll = urlBar->height;

    // TODO: load serialized tabs from disk
    allTabs.push_back(webView);

    // wipe out all files in the pviews directory if it exists
    cleanPrivateFiles();

    // create some directory structures for favorites, history, and cache
    mkdir("./data", 0777);
    mkdir("./data/favorites", 0777);
    mkdir("./data/views", 0777);
    mkdir("./data/pviews", 0777);
    mkdir("./data/cache", 0777);

    // parse the favorites from json
    std::map<std::string, void*> map;
    parseJSON(readFile("./data/favorites.json"), map);

    // // print each key and value in the map
    for (auto const& x : map) {
        if (x.first == "favorites") {
            // our favorites array!
            std::vector<std::string> favs = *(std::vector<std::string>*)x.second;
            for (auto const& fav : favs) {
                favorites.push_back(fav);
            }
        }
    }
}

bool MainDisplay::process(InputEvents* event)
{
    if (RootDisplay::subscreen)
		return RootDisplay::subscreen->process(event);

    bool ret = Element::process(event);
    if (ret) return true;

    auto saveAndQuit = [this]() {
        cleanPrivateFiles();
        writeFile("./data/views.json", fullSessionSummary());
        writeFile("./data/favorites.json", favoritesSummary());
        requestQuit();
    };
    // keep processing child elements
    
    this->canUseSelectToExit = false; // we'll handle quitting logic ourselves
    if (event->pressed(SELECT_BUTTON)) {
        // save the current state and quit (TODO: prompt the user?)
        saveAndQuit();
        return true;
    }

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
    newTab->y = urlBar->height;
    newTab->minYScroll = urlBar->height;

    auto curTabs = getAllTabs();
    curTabs->push_back(newTab);
    return curTabs->size() - 1;
}

WebView* MainDisplay::getActiveWebView() {
    auto curTabs = getAllTabs();
    if (activeTabIndex >= curTabs->size()) {
        return allTabs[0]; // error handler, should always be here
    }
    return (*curTabs)[activeTabIndex];
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

    // delete two characters (trailing newline and last comma)
    ret = ret.substr(0, ret.size() - 2);

    ret += "\n\t]\n";
    ret += "}\n";

    return ret;
}

std::string MainDisplay::favoritesSummary() {
    std::string ret = "{\n";
    ret += "\t\"favorites\": [\n";
        
    bool addedOne = false;
    for (int i = 0; i < favorites.size(); i++) {
        ret += "\t\t\"" + favorites[i] + "\",\n";
        addedOne = true;
    }

    // delete two characters (trailing newline and last comma)
    if (addedOne)
        ret = ret.substr(0, ret.size() - 2);

    ret += "\n\t]\n";
    ret += "}\n";

    return ret;
}

std::vector<WebView*>* MainDisplay::getAllTabs() {
    if (privateMode) {
        return &privateTabs;
    }
    return &allTabs;
}

void MainDisplay::cleanPrivateFiles() {
    struct stat info;
    std::filesystem::path path = "./data/pviews";
    if (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
        // use C++ filesystem to delete all files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            std::filesystem::remove(entry.path());
        }
    }
}

void MainDisplay::goToStartPage() {
    // if our current tabs are empty, make a new one
    // if one of our tabs is the start page, use it as the active index
    int newIdx = -1;
    auto curTabs = *getAllTabs();
    for (int i = curTabs.size()-1; i >= 0; i--) {
        WebView* webView = curTabs[i];
        if (webView->url == START_PAGE) {
            newIdx = i;
            break;
        }
    }
    if (newIdx == -1) {
        // either no tabs, or no start page, so make a new one
        newIdx = createNewTab();
    }
    setActiveTab(newIdx);
}

void MainDisplay::restoreTabs() {
    // re-add the previously opened tabs as webviews
    std::map<std::string, void*> map;
    parseJSON(readFile("./data/views.json"), map);

    auto curTabs = getAllTabs();

    // print each key and value in the map
    for (auto const& x : map) {
        if (x.first == "views") {
            auto views = *(std::vector<std::map<std::string, void*>>*)x.second;
            for (auto const& view : views) {
                auto newView = new WebView();
                // auto idStr = *(std::string*)view["id"];
                // auto urlIndex = *(int*)view["urlIndex"];
                // auto urls = *(std::vector<std::string>*)view["urls"];
                // newView->id = idStr;
                // newView->historyIndex = urlIndex;
                // newView->history = urls.copy();
                curTabs->push_back(newView);
            }
        }
    }
}