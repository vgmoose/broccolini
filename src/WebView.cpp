#include "WebView.hpp"
#include "../utils/Utils.hpp"
#include "../utils/BrocContainer.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/NetImageElement.hpp"

#include <iostream>

WebView::WebView()
{
    // download the target page
    this->url = "vgmoose.com";
    // this->url = "wikipedia.org";
    // this->url = "serebii.net";
    // this->url = "cute puppies";
    // this->url = "https://en.wikipedia.org/wiki/Main_Page";
    // this->url = "https://maschell.github.io";
    // this->url = "https://www.w3.org/Style/Examples/007/fonts.en.html";
    // this->url = "https://www.w3.org/Style/CSS/Test/CSS1/current/test5526c.htm";
    // this->url = "http://acid2.acidtests.org/#top";
    // this->url = "http://acid3.acidtests.org";
    needsLoad = true;

    this->width = RootDisplay::screenWidth;
    this->height = RootDisplay::screenHeight;

    // all webviews are assumed to be full screen
    RootDisplay::mainDisplay->windowResizeCallback = [this]() {
        this->width = RootDisplay::screenWidth;
        this->height = RootDisplay::screenHeight;
        this->needsRender = true;
        this->needsRedraw = true;
    };
}

bool WebView::process(InputEvents *event)
{
    if (needsLoad) {
        this->downloadPage();
        needsLoad = false;
        return true;
    }
    // ((NetImageElement*)elements[0])->load();
    // keep processing child elements
    return ListElement::processUpDown(event) || ListElement::process(event);
}

void WebView::render(Element *parent)
{
    if (needsRender) {
        this->m_doc->render(this->width);
    }

    if (container != nullptr) {
        litehtml::position posObj = litehtml::position(0, 0, RootDisplay::screenWidth, RootDisplay::screenHeight);
        this->m_doc->draw((litehtml::uint_ptr)container, this->x, this->y, &posObj);
    }

    // render the child elements (above whatever we just drew)
    ListElement::render(parent);
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
        if (url.find(".") == std::string::npos) {
            // no dots, let's assume it's a search query
            url = "https://google.com/search?q=" + url;
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

void WebView::handle_http_code(int httpCode, std::map<std::string, std::string> headerResp) {
    std::cout << "Got non 200 HTTP code: " << httpCode << std::endl;
    if (httpCode == 301 || httpCode == 302) {
        // redirect (TODO: cache?)
        this->url = sanitize_url(headerResp["location"], this->url.find("https://") != std::string::npos);
        redirectCount ++;
        downloadPage();
    }
}

void WebView::downloadPage()
{
    this->url = sanitize_url(this->url);
    std::cout << "Downloading page: " << this->url << std::endl;

    // download the page
    this->contents = "";

    int httpCode = 0;
    std::map<std::string, std::string> headerResp;

    downloadFileToMemory(this->url, &this->contents, &httpCode, &headerResp);

    if (redirectCount > 10) {
        std::cout << "Too many redirects, aborting" << std::endl;
        this->contents = "<html><body><h1>Too many redirects</h1></body></html>";
    }
    else if (httpCode != 200 && httpCode != 0) {
        handle_http_code(httpCode, headerResp);
        return;
    }

    // std::cout << "Contents: " << this->contents << std::endl;

    // litehtml::context ctx;
    // ctx.load_master_stylesheet(RAMFS "master.css");

    container = new BrocContainer(this);
    container->set_base_url(this->url.c_str());

    this->m_doc = litehtml::document::createFromString(this->contents.c_str(), container);
    this->needsRender = true;

}
