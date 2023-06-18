#include "../libs/chesto/src/Element.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "../libs/chesto/src/RootDisplay.hpp"
#include "./WebView.hpp"
#include <unordered_map>

// TODO: no forward declaration
class URLBar;

class MainDisplay : public RootDisplay
{
public:
	MainDisplay();
	bool process(InputEvents* event);
	void render(Element* parent);
	int mainLoop();

	URLBar* urlBar = NULL;
	std::vector<WebView*> allTabs;
	int activeTabIndex = 0;

	std::vector<std::string> favorites;

	WebView* setActiveTab(int index);
	int createNewTab();
	WebView* getActiveWebView();
	std::string fullSessionSummary();
};