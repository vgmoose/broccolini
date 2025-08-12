#include "../libs/chesto/src/DownloadQueue.hpp"
#include "../libs/chesto/src/Element.hpp"
#include "../utils/Utils.hpp"
#include "MainDisplay.hpp"
#include <algorithm>

int main(int argc, char* argv[])
{
	init_networking();

	// initialize main title screen
	MainDisplay* display = new MainDisplay();

	// if a URL was provided as a command line argument, set it for the first tab
	if (argc > 1)
	{
		std::string initialUrl = argv[1];
		// Get the first tab (WebView) and set its URL
		if (!display->allTabs.empty())
		{
			display->allTabs[0]->url = initialUrl;
			display->allTabs[0]->needsLoad = true;
		}
	}

	return display->mainLoop();
}