#include "../libs/chesto/src/Element.hpp"
#include <unordered_map>
#include <string>

class WebView : public Element
{
public:
	WebView();
    std::string url;
    std::string contents;
    void downloadPage();

    bool process(InputEvents* event);
    // void render(Element* parent);
};