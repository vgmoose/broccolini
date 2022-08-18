#include "WebView.hpp"
#include "../utils/Utils.hpp"
#include "../utils/BrocContainer.hpp"
#include <iostream>

WebView::WebView()
{
    // download the target page
    this->url = "https://vgmoose.com";
    this->downloadPage();
    
    litehtml::context ctx;
    ctx.load_master_stylesheet(RAMFS "master.css");

    BrocContainer* container = new BrocContainer();

    this->m_doc = litehtml::document::createFromString(this->contents.c_str(), container, &ctx);
}

bool WebView::process(InputEvents* event)
{
	// keep processing child elements
	return Element::process(event);
}

void WebView::render(Element* parent)
{
}

void WebView::downloadPage()
{
    std::cout << "Downloading page: " << this->url << std::endl;

    // download the page
    this->contents = "";
    downloadFileToMemory(this->url, &this->contents);

    std::cout << "Contents: " << this->contents << std::endl;
}