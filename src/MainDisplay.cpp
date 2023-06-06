#include "MainDisplay.hpp"
#include "WebView.hpp"


MainDisplay::MainDisplay()
{
	RootDisplay::super();
    // backgroundColor = fromRGB(0x3c, 0xb0, 0x43);
	backgroundColor = fromRGB(0xff, 0xff, 0xff);
}

bool MainDisplay::process(InputEvents* event)
{
	// keep processing child elements
    this->canUseSelectToExit = true;
	return RootDisplay::process(event);
}

int MainDisplay::mainLoop()
{
	WebView* webview = new WebView();
    this->child(webview);

	int ret = RootDisplay::mainLoop();

	return ret;
}