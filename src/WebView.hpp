#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "../libs/chesto/src/ListElement.hpp"
#include <litehtml.h>
#include <map>
#include <string>

#define START_PAGE "special://home"
#define SEARCH_URL "https://html.duckduckgo.com/html?q="

// TODO: no forward declare
class BrocContainer;
class JSEngine;
class VirtualDOM;

class WebView : public ListElement
{
public:
    WebView();
    ~WebView();
    std::string url;
    std::string contents;
    litehtml::document::ptr m_doc;

    std::vector<std::string> history;
    int historyIndex = -1;

    BrocContainer* container = nullptr;
    BrocContainer* prevContainer = nullptr;
    int redirectCount = 0;

    float zoomLevel = 1; // 100% zoom

    // JavaScript engine for executing scripts
    JSEngine* jsEngine = nullptr;
    
    // Virtual DOM manager
    VirtualDOM* virtualDOM = nullptr;
    
    // Flag to enable/disable JavaScript execution
    bool jsEnabled = true;

    // a vector of all the rectangles of the currently being clicked link
    std::vector<CST_Rect> nextLinkRects;

    // the "next" url to load if a touch event is successful (receives down and up with no movement in between)
    std::string nextLinkHref = "";
    std::string windowTitle = "";
    
    // Flag to track if title was set dynamically (to prevent HTML <title> from overriding)
    bool titleSetDynamically = false;

    // a unique ID that identifies this view, used by caches
    std::string id = "";

    // the color of the window border (can be set via <meta> tags)
	CST_Color theme_color = { 0xdd, 0xdd, 0xdd, 0xff };

    void downloadPage();
    bool handle_http_code(int httpCode, std::map<std::string, std::string> headerResp);

    bool process(InputEvents *event);
    void render(Element *parent);

    bool needsLoad = true;
    bool needsRender = true;

    std::string fullSessionSummary();
    // void screenshotPage();
    void screenshot(std::string path);
    
    // JavaScript-related methods
    void initializeJavaScript();
    void executePageScripts();
    void executeScriptsFromDocument();
    bool executeJavaScript(const std::string& script);
    void cleanupJavaScript();
    
    // Method to recreate document from modified contents
    void recreateDocument();

    void setTitle(const std::string& title);
    
    // Navigation methods
    void goTo(const std::string& url);
    void updateStorageDomain(); // Update cookie and localStorage domain from current URL
    void reloadPage();
};
#endif // WEBVIEW_H