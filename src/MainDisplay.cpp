#include <iostream>
#include "MainDisplay.hpp"
#include "WebView.hpp"
#include "URLBar.hpp"
#include "JSEngine.hpp"
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

    // create some directory structures for favorites, history, cache, and domains
    mkdir("./data", 0777);
    mkdir("./data/favorites", 0777);
    mkdir("./data/views", 0777);
    mkdir("./data/pviews", 0777);
    mkdir("./data/cache", 0777);
    mkdir("./data/domains", 0777);

    // parse the favorites from JSON using JSEngine directly
    // TODO: move out / extract method
    std::string favoritesContent = readFile("./data/favorites.json");
    if (!favoritesContent.empty()) {
        std::cout << "Attempting to parse favorites.json with JSEngine..." << std::endl;
        
        // pass a nullptr to use JSengine without a specific tab/webvie backing it
        JSEngine jsonEngine(nullptr);
        
        if (jsonEngine.parseJSON(favoritesContent)) {
            // extract the "favorites" array from the parsed JSON
            // format is: {"favorites": ["url1", "url2", ...]}
            std::vector<std::string> favs = jsonEngine.getArrayFromGlobal("parsedJSON", "favorites");
            
            if (!favs.empty()) {
                favorites = favs;
                std::cout << "Loaded " << favorites.size() << " favorites successfully using JSEngine!" << std::endl;
            } else {
                std::cout << "No favorites found in JSON or array is empty" << std::endl;
                if (!jsonEngine.getLastError().empty()) {
                    std::cout << "JSEngine error: " << jsonEngine.getLastError() << std::endl;
                }
            }
        } else {
            std::cout << "Failed to parse favorites.json: " << jsonEngine.getLastError() << std::endl;
        }
    } else {
        std::cout << "No favorites.json file found or empty" << std::endl;
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
    // Build JSON structure properly using muJS
    js_State* J = js_newstate(NULL, NULL, JS_STRICT);
    js_pushundefined(J);
    js_pushundefined(J);
    js_pushundefined(J);
    js_pushglobal(J);
    
    // Create main object
    js_newobject(J);
    
    // Create views array
    js_newarray(J);
    
    // Add each webview
    for (size_t i = 0; i < allTabs.size(); i++) {
        WebView* webView = allTabs[i];
        js_newobject(J);
        
        // Add properties
        js_pushstring(J, webView->id.c_str());
        js_setproperty(J, -2, "id");
        
        js_pushnumber(J, webView->historyIndex);
        js_setproperty(J, -2, "urlIndex");
        
        // Add urls array
        js_newarray(J);
        for (size_t j = 0; j < webView->history.size(); j++) {
            js_pushstring(J, webView->history[j].c_str());
            js_setindex(J, -2, j);
        }
        js_setproperty(J, -2, "urls");
        
        // Set this view object in the views array
        js_setindex(J, -2, i);
    }
    
    // Set views array in main object
    js_setproperty(J, -2, "views");
    
    // Stringify the result
    js_getglobal(J, "JSON");
    js_getproperty(J, -1, "stringify");
    js_copy(J, -3); // copy our object
    js_call(J, 1);
    
    std::string result = js_tostring(J, -1);
    js_freestate(J);
    
    return result;
}

std::string MainDisplay::favoritesSummary() {
    // Use JSEngine for JSON stringification instead of MuJSUtils
    JSEngine jsonEngine(nullptr);
    js_State* J = jsonEngine.getState();
    
    // Create main object
    js_newobject(J);
    
    // Create favorites array
    js_newarray(J);
    for (size_t i = 0; i < favorites.size(); i++) {
        js_pushstring(J, favorites[i].c_str());
        js_setindex(J, -2, i);
    }
    js_setproperty(J, -2, "favorites");
    
    // Stringify the result
    js_getglobal(J, "JSON");
    js_getproperty(J, -1, "stringify");
    js_copy(J, -3); // copy our object
    js_call(J, 1);
    
    std::string result = js_tostring(J, -1);
    return result;
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
    // re-add the previously opened tabs as webviews using JSEngine JSON parsing
    std::string viewsContent = readFile("./data/views.json");
    if (viewsContent.empty()) return;
    
    JSEngine jsonEngine(nullptr);
    if (!jsonEngine.parseJSON(viewsContent)) {
        std::cout << "Failed to parse views.json: " << jsonEngine.getLastError() << std::endl;
        return;
    }
    
    js_State* J = jsonEngine.getState();
    js_getglobal(J, "parsedJSON");
    
    if (!js_isobject(J, -1)) {
        js_pop(J, 1);
        return;
    }
    
    // Get the "views" array
    js_getproperty(J, -1, "views");
    if (!js_isarray(J, -1)) {
        js_pop(J, 2);
        return;
    }
    
    auto curTabs = getAllTabs();
    int arrayLength = js_getlength(J, -1);
    
    for (int i = 0; i < arrayLength; i++) {
        js_getindex(J, -1, i);
        if (js_isobject(J, -1)) {
            auto newView = new WebView();
            
            // Extract id
            js_getproperty(J, -1, "id");
            if (js_isstring(J, -1)) {
                newView->id = js_tostring(J, -1);
            }
            js_pop(J, 1);
            
            // Extract urlIndex
            js_getproperty(J, -1, "urlIndex");
            if (js_isnumber(J, -1)) {
                newView->historyIndex = js_tonumber(J, -1);
            }
            js_pop(J, 1);
            
            // Extract urls array
            js_getproperty(J, -1, "urls");
            if (js_isarray(J, -1)) {
                int urlsLength = js_getlength(J, -1);
                for (int j = 0; j < urlsLength; j++) {
                    js_getindex(J, -1, j);
                    if (js_isstring(J, -1)) {
                        newView->history.push_back(js_tostring(J, -1));
                    }
                    js_pop(J, 1);
                }
            }
            js_pop(J, 1); // pop urls array
            
            curTabs->push_back(newView);
        }
        js_pop(J, 1); // pop view object
    }
    
    js_pop(J, 2); // pop views array and parsedJSON
}