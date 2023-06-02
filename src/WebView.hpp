#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "../libs/chesto/src/ListElement.hpp"
#include <litehtml.h>
#include <unordered_map>
#include <string>

// TODO: no forward declare
class BrocContainer;

class WebView : public ListElement
{
public:
    WebView();
    std::string url;
    std::string contents;
    litehtml::document::ptr m_doc;

    BrocContainer* container = nullptr;

    void downloadPage();

    bool process(InputEvents *event);
    void render(Element *parent);

    bool needsLoad = true;
};
#endif // WEBVIEW_H