#include "../libs/chesto/src/Element.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/RootDisplay.hpp"
#include "../libs/chesto/src/TextElement.hpp"
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
	std::vector<WebView*> privateTabs;

	int activeTabIndex = 0;
	bool privateMode = false;

	std::vector<std::string> favorites;
	void cleanPrivateFiles();

	WebView* setActiveTab(int index);
	int createNewTab();

	WebView* getActiveWebView();
	std::vector<WebView*>* getAllTabs();
	void goToStartPage();

	std::string fullSessionSummary();
	std::string favoritesSummary();

	void restoreTabs();
};