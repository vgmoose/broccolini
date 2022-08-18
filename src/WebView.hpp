#include "../libs/chesto/src/Element.hpp"
#include <litehtml.h>
#include <unordered_map>
#include <string>

class WebView : public Element
{
public:
	WebView();
    std::string url;
    std::string contents;
    litehtml::document::ptr m_doc;

    void downloadPage();

    bool process(InputEvents* event);
    void render(Element* parent);
};