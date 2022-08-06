#include "WebView.hpp"
#include "../utils/Utils.hpp"
#include <iostream>

WebView::WebView()
{
    // download the target page
    this->url = "https://vgmoose.com";
    this->downloadPage();

    // this->elements.append(new TextElement(this->contents));
}

bool WebView::process(InputEvents* event)
{
	// keep processing child elements
	return Element::process(event);
}

// void WebView::render(Element* parent)
// {
    
// }

void WebView::downloadPage()
{
    std::cout << "Downloading page: " << this->url << std::endl;

    // download the page
    this->contents = "";
    downloadFileToMemory(this->url, &this->contents);

    std::cout << "Contents: " << this->contents << std::endl;
    
    // parse the page
    // this->parsePage();
}