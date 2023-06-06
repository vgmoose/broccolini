#include "BrocContainer.hpp"
#include "NetImageElement.hpp"
#include "ImageElement.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iostream>

// This class implements the litehtml::document_container interface.
// And hooks up litehtml methods to SDL/Chesto methods.

BrocContainer::BrocContainer(WebView* webView)
{
    this->webView = webView;
    this->font = CST_CreateFont();  

    auto fontPath = RAMFS "./res/opensans.ttf";
    auto renderer = (RootDisplay::mainDisplay)->renderer;
    CST_LoadFont(font, renderer, fontPath, get_default_font_size(), CST_MakeColor(0,0,0,255), TTF_STYLE_NORMAL); 
    
}

litehtml::uint_ptr BrocContainer::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
    // std::cout << "Requested to create font: " << faceName << std::endl;
    fm->ascent = 0;
    fm->descent = 0;
    fm->height = get_default_font_size();
    fm->x_height = get_default_font_size();

    return 1;
}

void BrocContainer::delete_font(litehtml::uint_ptr hFont) {
    std::cout << "Deleting font: " << hFont << std::endl;
}

int BrocContainer::text_width(const char* text, litehtml::uint_ptr hFont) {
    // printf("Some text: %s\n", text);
    return  CST_GetFontWidth(font, text);
}

void BrocContainer::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {
    // std::cout << "Requested to render text: [" << text << "]" << std::endl;
    auto renderer = RootDisplay::mainDisplay->renderer;
    auto size = get_default_font_size();
    CST_DrawFont(this->font, renderer, pos.left(), pos.bottom(), text);
    // eternalCounter += size;
}

int BrocContainer::pt_to_px(int pt) const {
    return pt;
}

int BrocContainer::get_default_font_size() const {
    return 20;
}

const char* BrocContainer::get_default_font_name() const {
    return "sans-serif";
}

void BrocContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) {
    // printf("Requested to render list marker\n");
}

// convert the url component according to URI rules
std::string BrocContainer::resolve_url(const char* src, const char* baseurl) {
    std::stringstream ss;
    std::string url;

    if (baseurl == 0) {
        if (strlen(src) > 0 && src[0] == '/') {
            if (strlen(src) > 1 && src[1] == '/') {
                // this is a starting double "//" url, just take the protocol
                ss << this->protocol << ":" << src;
            } else {
                // this is a starting single "/" url, just take the domain
                ss << this->base_domain << src;
            }

        } else {
            // this is a relative url, take the base url
            ss << this->base_url << "/" << src;
        }

        url = ss.str();
    }
    return url;
}

void BrocContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready) {

    std::string newUrl = resolve_url(src, baseurl);
    
    // create an ImageElement
    std::cout << "Requested to render Image [" << newUrl << "]" << std::endl;
    NetImageElement* img = new NetImageElement(
        newUrl.c_str(),
        []() {
            // could not load image, fallback
            return new ImageElement(RAMFS "res/redx.png");
        }
    );
    // ImageElement* img = new ImageElement(RAMFS "res/redx.png");

    img->position(10, 10);
    img->updateSizeAfterLoad = true;
    webView->child(img);
}

void BrocContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz) {
    printf("Requested image size for: %s\n", src);
}

void BrocContainer::draw_background( litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bgvec ) {
    printf("Requested to draw background\n");
    auto renderer = RootDisplay::mainDisplay->renderer;
    
    // const char* skirmie = bg.image.c_str();
    // printf("Requesting image from %s, at %d, %d\n", skirmie, bg.clip_box.x, bg.clip_box.y);
}

void BrocContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) {

    CST_Rect dimens = {
        draw_pos.left(),
        draw_pos.top(),
        draw_pos.right() - draw_pos.left(),
        draw_pos.bottom() - draw_pos.top()
    };

    printf("Border drawn at: %d, %d, %d, %d\n", dimens.x, dimens.y, dimens.w, dimens.h);

    auto renderer = RootDisplay::mainDisplay->renderer;
    CST_SetDrawColorRGBA(renderer, 0, 0, 0, 255);
    CST_DrawRect(renderer, &dimens);
}

void BrocContainer::set_caption(const char* caption ) {
    // std::cout << "Setting caption to: " << caption << std::endl;
    CST_SetWindowTitle(caption);
}

void BrocContainer::set_base_url(const char* base_url ) {
    // std::cout << "Setting base url to: " << base_url << std::endl;
    std::string url = base_url;

    // extract base url from full URL    
    this->base_url = url.substr(0, url.find_last_of("/"));

    // extract base domain from full URL
    url += "/";
    std::string protocol = url.substr(0, url.find("://"));
    std::string domain = url.substr(url.find("://") + 3);
    domain = domain.substr(0, domain.find("/"));
    this->base_domain = protocol + "://" + domain;

    this->protocol = protocol;
}

void BrocContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el ) {
    auto href = el->get_attr("href");
    auto rel = el->get_attr("rel");
    std::cout << "<link> tag detected for href: " << href << " with rel: " << rel << std::endl;
}

void BrocContainer::on_anchor_click(const char* url, const litehtml::element::ptr& el ) {
    std::cout << "Anchor clicked: " << url << std::endl;
}

void BrocContainer::set_cursor(const char* cursor ) {
    std::cout << "Setting cursor to: " << cursor << std::endl;
}

void BrocContainer::transform_text(litehtml::string& text, litehtml::text_transform tt ) {
    std::cout << "Transforming text: " << text << std::endl;
}

void BrocContainer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl ) {
    // std::cout << "Importing CSS: " << text << ", " << url << std::endl;
    std::string newUrl = resolve_url(url.c_str(), baseurl.c_str());

    /// download the CSS file
    std::string contents;
    downloadFileToMemory(newUrl.c_str(), &text);
}

void BrocContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) {
    printf("Setting clip\n");
}

void BrocContainer::del_clip( ) {
    printf("Deleting clip\n");
}

void BrocContainer::get_client_rect(litehtml::position& client ) const {
    // fill client rect with viewport size
    // printf("Getting client rect\n");
    client.width = RootDisplay::screenWidth;
    client.height = RootDisplay::screenHeight;
}

std::shared_ptr<litehtml::element> BrocContainer::create_element(const char *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc) {
    // std::cout << "Requested to create an element: " << tag_name << std::endl;

    // // print out each attribute
    // for (auto& attr : attributes) {
    //     std::cout << "  " << attr.first << " = " << attr.second << std::endl;
    // }

    // get the content from the litehtml document
    // auto content = doc->get_text();
    // std::cout << "  Content: " << content << std::endl;

    return 0;
}

void BrocContainer::get_media_features(litehtml::media_features& media ) const {
    // printf("Getting media features\n");
    media.width = RootDisplay::screenWidth;
    media.height = RootDisplay::screenHeight;
    media.device_width = RootDisplay::screenWidth;
    media.device_height = RootDisplay::screenHeight;
    media.color = 8;
    media.monochrome = 0;
    media.color_index = 256;
    media.resolution = 96;
}

void BrocContainer::get_language(litehtml::string& language, litehtml::string & culture ) const {
    printf("Getting language\n");
}
