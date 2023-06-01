#include "../libs/chesto/src/Element.hpp"
#include "../utils/Utils.hpp"
#include "MainDisplay.hpp"
#include "../libs/chesto/src/DownloadQueue.hpp"
#include <algorithm>

int main(int argc, char* argv[])
{
    init_networking();

	// initialize main title screen
	MainDisplay* display = new MainDisplay();

	return display->mainLoop();
}