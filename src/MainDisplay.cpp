#include "MainDisplay.hpp"

MainDisplay::MainDisplay()
{
	RootDisplay::super();
    backgroundColor = fromRGB(0x3c, 0xb0, 0x43);
}

bool MainDisplay::process(InputEvents* event)
{
	// keep processing child elements
    this->canUseSelectToExit = true;
	return RootDisplay::process(event);
}
