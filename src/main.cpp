#include "../libs/chesto/src/Element.hpp"
#include "../utils/Utils.hpp"
#include "MainDisplay.hpp"
#include "WebView.hpp"
#include <algorithm>

int main(int argc, char* argv[])
{
    init_networking();

	// initialize main title screen
	MainDisplay* display = new MainDisplay();

    WebView* webview = new WebView();
    display->elements.push_back(webview);

	return display->mainLoop();
}