#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "../libs/chesto/src/ListElement.hpp"
#include <litehtml.h>
#include <map>
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
    int redirectCount = 0;

    // the "next" element to click touch event is successful (receives down and up with no movement in between)
    CST_Rect nextLinkOverlay = {0, 0, 0, 0};
    std::string nextLinkHref = "";

    void downloadPage();
    void handle_http_code(int httpCode, std::map<std::string, std::string> headerResp);

    bool process(InputEvents *event);
    void render(Element *parent);

    bool needsLoad = true;
    bool needsRender = true;
};
#endif // WEBVIEW_H