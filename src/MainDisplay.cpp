#include "MainDisplay.hpp"
#include "WebView.hpp"
#include "URLBar.hpp"

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
	WebView* webView = new WebView();
	urlBar = new URLBar(webView);

	// webView->y = urlBar->height;

    child(webView);
	child(urlBar);

	// for when we resize the main window, adjust some sizes
    RootDisplay::mainDisplay->windowResizeCallback = [this, webView]() {
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