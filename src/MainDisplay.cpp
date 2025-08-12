#include "MainDisplay.hpp"
#include "../utils/Utils.hpp"
#include "JSEngine.hpp"
#include "URLBar.hpp"
#include "WebView.hpp"
#include <filesystem>
#include <iostream>
#include <sstream> // added for std::istringstream
#include <sys/stat.h>

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
	std::string favoritesContent = readFile("./data/favorites.json");
	if (!favoritesContent.empty())
	{
		std::cout << "Attempting to parse favorites.json with JSEngine..."
				  << std::endl;

		// pass a nullptr to use JSEngine without a specific tab/webview backing it
		auto jsonEngine = JSEngine::create(nullptr);

		if (jsonEngine && jsonEngine->parseJSON(favoritesContent))
		{
			// For now, skip favorites parsing with simplified engine
			std::cout << "JSON parsed successfully, but array extraction not implemented in simplified interface" << std::endl;
		}
		else
		{
			std::cout << "Failed to parse favorites.json with JSEngine" << std::endl;
			if (jsonEngine && !jsonEngine->getLastError().empty())
			{
				std::cout << "JSEngine error: " << jsonEngine->getLastError() << std::endl;
			}
		}
	}
	else
	{
		std::cout << "No favorites.json file found or empty" << std::endl;
	}
}

bool MainDisplay::process(InputEvents* event)
{
	if (RootDisplay::subscreen)
		return RootDisplay::subscreen->process(event);

	bool ret = Element::process(event);
	if (ret)
		return true;

	auto saveAndQuit = [this]()
	{
		cleanPrivateFiles();
		writeFile("./data/views.json", fullSessionSummary());
		writeFile("./data/favorites.json", favoritesSummary());
		requestQuit();
	};
	// keep processing child elements

	this->canUseSelectToExit = false; // we'll handle quitting logic ourselves
	if (event->pressed(SELECT_BUTTON))
	{
		// save the current state and quit (TODO: prompt the user?)
		saveAndQuit();
		return true;
	}

	// manually process events for active tab and url bar

	// if we get a url bar press, don't continue
	if (urlBar->process(event))
	{
		return true;
	}

	// if the active tab is interacted with, don't continue
	WebView* activeTab = getActiveWebView();
	if (activeTab != nullptr && activeTab->process(event))
	{
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

	// we don't actually have children elements, manually render the active tab
	// and the url bar
	WebView* activeTab = getActiveWebView();
	if (activeTab != nullptr)
	{
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
int MainDisplay::createNewTab()
{
	// create a new webview, append it to all tabs
	WebView* newTab = new WebView();
	newTab->y = urlBar->height;
	newTab->minYScroll = urlBar->height;

	auto curTabs = getAllTabs();
	curTabs->push_back(newTab);
	return curTabs->size() - 1;
}

WebView* MainDisplay::getActiveWebView()
{
	auto curTabs = getAllTabs();
	if (activeTabIndex >= curTabs->size())
	{
		return allTabs[0]; // error handler, should always be here
	}
	return (*curTabs)[activeTabIndex];
}

int MainDisplay::mainLoop()
{
	// for when we resize the main window, adjust some sizes
	RootDisplay::mainDisplay->windowResizeCallback = [this]()
	{
		auto webView = this->getActiveWebView();
		webView->width = RootDisplay::screenWidth;
		webView->height = RootDisplay::screenHeight;
		webView->needsRender = true;
		webView->needsRedraw = true;

		urlBar->width = RootDisplay::screenWidth;
		urlBar->recalcPosition(this);

		urlBar->futureRedrawCounter = 10; // redraw the URL bar for a few frames

		// TODO: put all images off-screen so that they won't be in the wrong place
		// temporarily
	};

	int ret = RootDisplay::mainLoop();

	return ret;
}

std::string MainDisplay::fullSessionSummary()
{
	std::string result = "{\n  \"views\": [";
	for (size_t i = 0; i < allTabs.size(); ++i)
	{
		WebView* w = allTabs[i];
		result += "{\"id\":\"" + w->id + "\",\"urlIndex\":" + std::to_string(w->historyIndex) + ",\"urls\":[";
		for (size_t j = 0; j < w->history.size(); ++j)
		{
			result += "\"" + w->history[j] + "\"";
			if (j + 1 < w->history.size())
				result += ",";
		}
		result += "]}";
		if (i + 1 < allTabs.size())
			result += ",";
	}
	result += "]\n}";
	return result;
}

std::string MainDisplay::favoritesSummary()
{
	std::string result = "{\"favorites\":[";
	for (size_t i = 0; i < favorites.size(); ++i)
	{
		result += "\"" + favorites[i] + "\"";
		if (i + 1 < favorites.size())
			result += ",";
	}
	result += "]}";
	return result;
}

void MainDisplay::cleanPrivateFiles()
{
	struct stat info;
	std::filesystem::path path = "./data/pviews";
	if (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
	{
		// use C++ filesystem to delete all files in the directory
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			std::filesystem::remove(entry.path());
		}
	}
}

void MainDisplay::goToStartPage()
{
	// if our current tabs are empty, make a new one
	// if one of our tabs is the start page, use it as the active index
	int newIdx = -1;
	auto curTabs = *getAllTabs();
	for (int i = curTabs.size() - 1; i >= 0; i--)
	{
		WebView* webView = curTabs[i];
		if (webView->url == START_PAGE)
		{
			newIdx = i;
			break;
		}
	}
	if (newIdx == -1)
	{
		// either no tabs, or no start page, so make a new one
		newIdx = createNewTab();
	}
	setActiveTab(newIdx);
}

void MainDisplay::restoreTabs()
{
	std::string viewsContent = readFile("./data/views.json");
	if (viewsContent.empty())
		return;

	// Spin up a temporary JS engine for parsing
	auto parser = JSEngine::create(nullptr);
	if (!parser)
		return;

	// Build a JS script that safely parses and serializes essential data into a
	// compact string Format per view: id|urlIndex|url1~url2~url3 separated by
	// newline We wrap JSON in parentheses and avoid injecting by not executing
	// arbitrary properties.
	std::string script = "var __viewsParsed='';";
	script += "try { const __data = (" + viewsContent + ");";
	script += "if(__data && Array.isArray(__data.views)){ for (var "
			  "i=0;i<__data.views.length;i++){ var v=__data.views[i]; if(!v) "
			  "continue; var urls=(v.urls||[]).join('~'); __viewsParsed += "
			  "(i?'\n':'') + [v.id||'', v.urlIndex||0, urls].join('|'); }}";
	script += "} catch(e) { __viewsParsed=''; }";
	parser->executeScript(script);

	std::string packed = parser->getGlobalString("__viewsParsed");
	if (packed.empty())
		return;

	std::istringstream iss(packed);
	std::string line;
	while (std::getline(iss, line))
	{
		if (line.empty())
			continue;
		// Split line by '|'
		size_t p1 = line.find('|');
		if (p1 == std::string::npos)
			continue;
		size_t p2 = line.find('|', p1 + 1);
		if (p2 == std::string::npos)
			continue;
		std::string id = line.substr(0, p1);
		int urlIndex = std::stoi(line.substr(p1 + 1, p2 - (p1 + 1)));
		std::string urlsPart = line.substr(p2 + 1);
		std::vector<std::string> urls;
		size_t start = 0;
		while (start <= urlsPart.size())
		{
			size_t sep = urlsPart.find('~', start);
			if (sep == std::string::npos)
			{
				urls.push_back(urlsPart.substr(start));
				break;
			}
			urls.push_back(urlsPart.substr(start, sep - start));
			start = sep + 1;
		}
		if (urls.size() == 1 && urls[0].empty())
			urls.clear();
		auto newView = new WebView();
		newView->id = id;
		newView->historyIndex = urlIndex;
		newView->history = urls;
		getAllTabs()->push_back(newView);
	}
}

std::vector<WebView*>* MainDisplay::getAllTabs()
{
	return privateMode ? &privateTabs : &allTabs;
}