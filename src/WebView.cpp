#include "WebView.hpp"
#include "../utils/Utils.hpp"
#include "../utils/BrocContainer.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/NetImageElement.hpp"
#include "MainDisplay.hpp"
#include "URLBar.hpp"
#include "../utils/UIUtils.hpp"
#include "JSEngine.hpp"
#include "VirtualDOM.hpp"

#include <iostream>
#include <string>
#include <regex>
#include <iterator>
#include <fstream>

WebView::WebView()
{
    this->url = START_PAGE;
    needsLoad = true;

    this->width = RootDisplay::screenWidth;
    this->height = RootDisplay::screenHeight;

    // put a random number into the ID string
    this->id = std::to_string(rand()) + std::to_string(rand());
    
    // Initialize JavaScript support
    initializeJavaScript();
}

WebView::~WebView() {
    cleanupJavaScript();
}

bool WebView::process(InputEvents* e)
{
    if (hidden) {
        return false;
    }
    if (needsLoad) {
        this->downloadPage();
        needsLoad = false;
        auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
        return true;
    }

    litehtml::position::vector redraw_boxes;

    if (e->pressed(A_BUTTON)) {
        zoomLevel += 0.1;
        printf("Zoom level: %f\n", zoomLevel);
        return true;
    }

    if (e->pressed(B_BUTTON)) {
        zoomLevel -= 0.1;
        printf("Zoom level: %f\n", zoomLevel);
        return true;
    }

    bool resp = false;

    if (e->isTouchDown()) {
        // we call up first so that we can get the element that will clicked, and display it as highlighted
        resp = this->m_doc->on_lbutton_up(
            -1*this->x + e->xPos, -1*this->y + e->yPos,
            e->xPos, e->yPos,
            redraw_boxes
        );
        resp = this->m_doc->on_lbutton_down(
            -1*this->x + e->xPos, -1*this->y + e->yPos,
            e->xPos, e->yPos,
            redraw_boxes
        );
        // printf("Got touch down with response %d\n", redraw_boxes.size());
    } else if (e->isTouchUp()) {
        resp = this->m_doc->on_lbutton_up(
            -1*this->x + e->xPos, -1*this->y + e->yPos,
            e->xPos, e->yPos,
            redraw_boxes
        );
        // printf("Got touch up with response %d\n", redraw_boxes.size());
    } else if (e->isTouchDrag()) {
        resp = this->m_doc->on_mouse_over(
            -1*this->x + e->xPos, -1*this->y + e->yPos,
            e->xPos, e->yPos,
            redraw_boxes
        );
        nextLinkHref = "";
        // printf("Got touch drag with response %d\n", redraw_boxes.size());
    }
    
    // TODO: how to use this?
    // bool litehtml::document::on_mouse_leave( position::vector& redraw_boxes );

    // keep processing child elements
    return ListElement::processUpDown(e) || ListElement::process(e) || resp;
}

void WebView::render(Element *parent)
{
    if (hidden) {
        return;
    }
    if (needsRender && this->m_doc != nullptr) {
        this->m_doc->render(this->width);
    }

    if (prevContainer != nullptr) {
        // printf("Freeing container\n");
        delete prevContainer;
        prevContainer = nullptr;
    }

    if (container != nullptr) {
        litehtml::position posObj = litehtml::position(0, 0, this->width, this->height);
        this->m_doc->draw(
            (litehtml::uint_ptr)container,
            this->x, this->y, &posObj
        );
    }

    // render the child elements (above whatever we just drew)
    ListElement::render(parent);

    // render the overlay of the next link, if it's set
    if (nextLinkHref != "") {
        // iterate through the areas in webView->nextLinkRects
        auto renderer = RootDisplay::mainDisplay->renderer;
        for (auto rect : nextLinkRects) {
            // draw a rectangle over the area
            CST_roundedBoxRGBA(renderer, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, 0,  0xad, 0xd8, 0xe6, 0x90);
            CST_roundedRectangleRGBA(renderer, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, 0,  0x66, 0x7c, 0x89, 0x90);
        }
    }
}

std::string uri_encode(std::string uri) {
    std::string encoded = "";
    for (char c : uri) {
        if (c == ' ') {
            encoded += "%20";
        } else {
            encoded += c;
        }
    }
    return encoded;
}

std::string sanitize_url(std::string url, bool isHTTPS = false)
{
    // trim any ending whitespace (or newlines)
    url.erase(0, url.find_first_not_of(" \n\r\t"));
    url.erase(url.find_last_not_of(" \n\r\t")+1);

    // if the base url doesn't have a protocol, add it with HTTP
    auto protoPos = url.find("://");
    if (protoPos == std::string::npos) {
        // no domain, let's see if we it's likely to be a search query (no dots)
        if (
            url.find(".") == std::string::npos
            && url.find(":") == std::string::npos
        ) {
            // no dots and no colons, let's assume it's a search query
            url = SEARCH_URL + url;
        } else {
            // no protocol
            if (isHTTPS) {
                url = "https://" + url;
            } else {
                url = "http://" + url;
            }
        }
    }

    // special case, if we have a single ending slash at the end of the domain (as the third slash), remove it
    if (url.find("://") != std::string::npos) {
        auto thirdSlash = url.find("/", url.find("://") + 3);
        if (thirdSlash != std::string::npos && thirdSlash == url.size() - 1) {
            url.erase(thirdSlash);
        }
    }

    // uri encode the url
    url = uri_encode(url);

    return url;
}

bool WebView::handle_http_code(int httpCode, std::map<std::string, std::string> headerResp) {
    if (this->contents == "") {
        if (httpCode == 404) {
            this->contents = load_special_page("not_found");
            return true;
        } else if (httpCode == 403) {
            this->contents = load_special_page("forbidden");
            return true;
        }
        this->contents = load_special_page("no_content", std::to_string(httpCode).c_str());
    }

    std::cout << "Got non 200 HTTP code: " << httpCode << std::endl;

    if (httpCode == 301 || httpCode == 302) {
        // redirect (TODO: cache?)
        this->url = sanitize_url(headerResp["location"], this->url.find("https://") != std::string::npos);
        auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
        mainDisplay->urlBar->currentUrl = this->url;
        mainDisplay->urlBar->updateInfo();
        redirectCount ++;
        downloadPage();
        return false;
    }

    return true;
}

void WebView::downloadPage()
{
    auto mainDisplay =  ((MainDisplay*)RootDisplay::mainDisplay);
    
    // if it's a mailto: link, display a message to open the mail app
    bool isMailto = this->url.find("mailto:") == 0;
    bool isSpecial = this->url.find("special:") == 0;

    this->url = sanitize_url(this->url);
    std::cout << "Downloading page: " << this->url << std::endl;

    // download the page
    this->contents = "";

    int httpCode = 0;
    std::map<std::string, std::string> headerResp;

    if (isMailto) {
        // TODO: extract all this special protocol detection logic
        this->contents = load_special_page("email_link", this->url.substr(7).c_str());
    } else if (isSpecial) {
        // extract the name of the special page
        std::string specialName = this->url.substr(10);
        if (specialName == "home") {
            // are we restoring a session?
            bool prevSession = true; // TODO: actually check for a previous session
            std::string restoreSession = prevSession ? load_special_page("restore_pane") : "";
            if (mainDisplay->privateMode) {
                restoreSession = load_special_page("private");
            }

            // load info from favorites
            auto favorites = mainDisplay->favorites;
            std::string favHtml = "";
            for (auto favUrl : favorites) {
                std::string favName = just_domain_from_url(favUrl);
                std::string favIcon = std::string("file://favorites/") + base64_encode(favUrl) + ".png";
                favHtml += load_special_page("favorite_card", favUrl.c_str(), favIcon.c_str(), favName.c_str());
            }

            if (favHtml == "") {
                favHtml = "You have no favorites yet! Use the star button to add one.";
            }

            // actually load the home page using these two html strings and the template
            this->contents = load_special_page("home", restoreSession.c_str(), favHtml.c_str());
        } else {
            this->contents = load_special_page(specialName);
        }
    } else {
        downloadFileToMemory(this->url, &this->contents, &httpCode, &headerResp);
    }

    if (redirectCount > 10) {
        std::cout << "Too many redirects, aborting" << std::endl;
        this->contents = load_special_page("too_many_redirects");
    }

    bool keepLoading = handle_http_code(httpCode, headerResp);

    if (!keepLoading) {        
        return;
    }

    // check for image mime type and use the image container template
    auto contentType = headerResp["content-type"];
    if (contentType.find("image/") == 0) {
        // convert contents to base64 image url
        std::string base64Contents = base64_encode(this->contents);
        base64Contents = "data:" + contentType + ";base64," + base64Contents;
        this->contents = load_special_page("image_container", base64Contents.c_str());
    }

    // std::cout << "Contents: " << this->contents << std::endl;

    // litehtml::context ctx;
    // ctx.load_master_stylesheet(RAMFS "master.css");

    // hack to show flex box containers
    // std::replace(this->contents.begin(), this->contents.end(), "flex", "block");
    // std::regex e ("\\b(sub)([^ ]*)");   // matches words beginning by "sub"
    // // using string/c-string (3) version:
    // std::cout << std::regex_replace (this->contents, e, "sub-$2");
    // std::cout << std::endl;

    // TODO: extract all the below rendering logic into another method

    if (container != nullptr) {
        // delete all children
        wipeAll();
        
        // TODO: crashes, deletes too early, but do this instead
        prevContainer = container;
    }

    // reset the theme color
    this->theme_color = { 0xdd, 0xdd, 0xdd, 0xff };

    container = new BrocContainer(this);
    // printf("Resetting container\n");
    container->set_base_url(this->url.c_str());

    this->m_doc = litehtml::document::createFromString(this->contents.c_str(), container);
    this->needsRender = true;

    // Execute JavaScript after document is loaded
    executePageScripts();

    // clear and append the history up to this point, if the current index is not the current url
    if (historyIndex < 0 || history[historyIndex] != this->url) {
        history.erase(history.begin() + historyIndex + 1, history.end());
        history.push_back(this->url);
        historyIndex = history.size() - 1;

        // show the clock
        auto urlBar = mainDisplay->urlBar;
        urlBar->clockButton->hidden = false;
        urlBar->forwardButton->hidden = true;
    }

    mainDisplay->urlBar->webView = this;
    mainDisplay->urlBar->currentUrl = this->url;
    mainDisplay->urlBar->updateInfo();
}

void WebView::screenshot(std::string path) {
    // offset by our top bound (such as a URL bar) before taking screenshot
    int origY = this->y;
    this->y -= this->minYScroll;
    Element::screenshot(path);
    this->y = origY;
}

std::string WebView::fullSessionSummary() {
    std::string summary = "\t\t{\n";
    summary += "\t\t\t\"id\": \"" + id + "\",\n";
    summary += "\t\t\t\"urlIndex\": " + std::to_string(historyIndex) + ",\n";
    summary += "\t\t\t\"urls\": \[\n";
    bool addedOne = false;
    for (auto url : history) {
        summary += "\t\t\t\t\"" + url + "\",\n";
        addedOne = true;
    }
    // remove the last comma
    if (addedOne)
        summary = summary.substr(0, summary.size() - 2);
    summary += "\n\t\t\t]\n\t\t}";
    return summary;
}

// JavaScript-related method implementations
void WebView::initializeJavaScript() {
    if (!jsEnabled) return;
    
    try {
        jsEngine = new JSEngine(this);
        virtualDOM = new VirtualDOM(this);
        
        if (virtualDOM->initializeSnabbdom()) {
            std::cout << "JavaScript engine initialized successfully" << std::endl;
        } else {
            std::cerr << "Failed to initialize Virtual DOM" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing JavaScript: " << e.what() << std::endl;
        cleanupJavaScript();
    }
}

void WebView::executePageScripts() {
    if (!jsEnabled || !jsEngine || !m_doc) return;
    
    std::cout << "Executing page scripts..." << std::endl;
    
    // Test basic JavaScript functionality
    executeJavaScript("console.log('JavaScript engine is working!');");
    
    // Find and execute script tags from the HTML document
    // We need to traverse the litehtml document to find script elements
    executeScriptsFromDocument();
}

void WebView::executeScriptsFromDocument() {
    if (!m_doc) return;
    
    // For now, we'll implement a basic script extraction
    // This is a simplified approach - in a full implementation, we'd properly traverse the DOM tree
    
    // Look for script content in the original HTML
    std::string html = this->contents;
    size_t pos = 0;
    
    while ((pos = html.find("<script", pos)) != std::string::npos) {
        // Find the end of the opening script tag
        size_t tagEnd = html.find(">", pos);
        if (tagEnd == std::string::npos) break;
        
        // Check if this is a script with type="text/javascript" or no type (default)
        std::string openingTag = html.substr(pos, tagEnd - pos + 1);
        bool isJavaScript = true;
        
        // Simple check for non-JavaScript script types
        if (openingTag.find("type=") != std::string::npos && 
            openingTag.find("javascript") == std::string::npos &&
            openingTag.find("text/javascript") == std::string::npos &&
            openingTag.find("application/javascript") == std::string::npos) {
            isJavaScript = false;
        }
        
        if (isJavaScript) {
            // Find the closing script tag
            size_t scriptEnd = html.find("</script>", tagEnd);
            if (scriptEnd != std::string::npos) {
                // Extract the script content
                std::string scriptContent = html.substr(tagEnd + 1, scriptEnd - tagEnd - 1);
                
                // Trim whitespace
                size_t start = scriptContent.find_first_not_of(" \t\n\r");
                size_t end = scriptContent.find_last_not_of(" \t\n\r");
                
                if (start != std::string::npos && end != std::string::npos) {
                    scriptContent = scriptContent.substr(start, end - start + 1);
                    
                    if (!scriptContent.empty()) {
                        std::cout << "Executing script: " << std::endl << scriptContent.substr(0, 100) << "..." << std::endl;
                        executeJavaScript(scriptContent);
                    }
                }
            }
        }
        
        pos = tagEnd + 1;
    }
}

bool WebView::executeJavaScript(const std::string& script) {
    if (!jsEnabled || !jsEngine) return false;
    
    bool success = jsEngine->executeScript(script);
    
    if (!success) {
        std::cerr << "JavaScript execution failed: " << jsEngine->getLastError() << std::endl;
    }
    
    return success;
}

void WebView::cleanupJavaScript() {
    if (virtualDOM) {
        delete virtualDOM;
        virtualDOM = nullptr;
    }
    
    if (jsEngine) {
        delete jsEngine;
        jsEngine = nullptr;
    }
}

void WebView::recreateDocument() {
    if (!container) return;
    
    std::cout << "Starting document recreation..." << std::endl;
    
    // Store the current state
    std::string currentUrl = this->url;
    
    // Clear the current document reference first
    this->m_doc = nullptr;
    
    // Now safely delete and recreate the container
    delete container;
    container = new BrocContainer(this);
    container->set_base_url(currentUrl.c_str());
    
    // Recreate the document from modified contents
    this->m_doc = litehtml::document::createFromString(this->contents.c_str(), container);
    this->needsRender = true;
    
    std::cout << "Successfully recreated litehtml document from modified contents" << std::endl;
    
    // Do NOT execute page scripts again as that would cause infinite loops
}