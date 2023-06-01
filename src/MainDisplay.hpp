#include "../libs/chesto/src/Element.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/RootDisplay.hpp"
#include <unordered_map>

class MainDisplay : public RootDisplay
{
public:
	MainDisplay();
	bool process(InputEvents* event);
	int mainLoop();
};