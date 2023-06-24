#include "../libs/chesto/src/Element.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/TextElement.hpp"
#include "./WebView.hpp"
#include <unordered_map>

// TODO: no forward declaration
class URLBar;

class TabSwitcher : public ListElement
{
public:
	TabSwitcher();
    void createTabCards();

	bool process(InputEvents* event);
	void render(Element* parent);

	Element* closeButton = NULL;
	Element* privateButton = NULL;
};