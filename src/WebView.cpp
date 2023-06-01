#include "WebView.hpp"
#include "../utils/Utils.hpp"
#include "../utils/BrocContainer.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/NetImageElement.hpp"

#include <iostream>

WebView::WebView()
{
    // download the target page
    this->url = "https://vgmoose.com";
    // this->url = "http://127.0.0.1:8000";
    needsLoad = true;
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
    return ListElement::process(event);
}

void WebView::render(Element *parent)
{
    // render the child elements
    ListElement::render(this);
}

void WebView::downloadPage()
{
    std::cout << "Downloading page: " << this->url << std::endl;

    // download the page
    this->contents = "";
    downloadFileToMemory(this->url, &this->contents);

    std::cout << "Contents: " << this->contents << std::endl;

    litehtml::context ctx;
    ctx.load_master_stylesheet(RAMFS "master.css");

    BrocContainer *container = new BrocContainer(this);
    container->set_base_url(this->url.c_str());

    this->m_doc = litehtml::document::createFromString(this->contents.c_str(), container, &ctx);
}
