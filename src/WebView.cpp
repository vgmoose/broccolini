#include "WebView.hpp"
#include "../utils/Utils.hpp"
#include "../utils/BrocContainer.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../libs/chesto/src/NetImageElement.hpp"

#include <iostream>

WebView::WebView()
{
    // download the target page
    // this->url = "https://vgmoose.com";
    this->url = "https://en.wikipedia.org/wiki/Main_Page";
    // this->url = "https://www.serebii.net";
    // this->url = "cute puppies";
    // this->url = "https://maschell.github.io";
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
    return ListElement::processUpDown(event) || ListElement::process(event);
}

void WebView::render(Element *parent)
{
    if (container != nullptr) {
        litehtml::position posObj = litehtml::position(0, 0, RootDisplay::screenWidth, RootDisplay::screenHeight);
        this->m_doc->draw((litehtml::uint_ptr)container, this->x, this->y, &posObj);
    }

    // render the child elements (above whatever we just drew)
    ListElement::render(parent);
}

void WebView::downloadPage()
{
    std::cout << "Downloading page: " << this->url << std::endl;

    // download the page
    this->contents = "";
    downloadFileToMemory(this->url, &this->contents);

    // std::cout << "Contents: " << this->contents << std::endl;

    // litehtml::context ctx;
    // ctx.load_master_stylesheet(RAMFS "master.css");

    container = new BrocContainer(this);
    container->set_base_url(this->url.c_str());

    this->m_doc = litehtml::document::createFromString(this->contents.c_str(), container);
    this->m_doc->render(RootDisplay::screenWidth);

}
