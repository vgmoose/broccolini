#include "../libs/chesto/src/Element.hpp"
#include "MainDisplay.hpp"
#include <algorithm>

int main(int argc, char* argv[])
{
	// initialize main title screen
	MainDisplay* display = new MainDisplay();
	display->mainLoop();

	return 0;
}