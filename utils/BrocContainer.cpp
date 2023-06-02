#include "BrocContainer.hpp"
#include "NetImageElement.hpp"
#include "ImageElement.hpp"
#include <iostream>

// This class implements the litehtml::document_container interface.
// And hooks up litehtml methods to SDL/Chesto methods.

BrocContainer::BrocContainer(WebView* webView)
{
    this->webView = webView;
    this->font = CST_CreateFont();  

    auto fontPath = RAMFS "./res/mono.ttf";
    auto renderer = (RootDisplay::mainDisplay)->renderer;
    CST_LoadFont(font, renderer, fontPath, get_default_font_size(), CST_MakeColor(0,0,0,255), TTF_STYLE_NORMAL); 
    
}

litehtml::uint_ptr BrocContainer::create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
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

int BrocContainer::text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont) {
    // printf("Some text: %s\n", text);
    return get_default_font_size();
}

void BrocContainer::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {
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

const litehtml::tchar_t* BrocContainer::get_default_font_name() const {
    return "sans-serif";
}

void BrocContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) {

}

void BrocContainer::load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready) {
    std::stringstream ss;
    std::string url;

    if (baseurl == 0) {
        ss << this->base_url << "/" << src;
        baseurl = this->base_url.c_str();
        url = ss.str();
        src = url.c_str();
    }
    
    // create an ImageElement
    std::cout << "Requested to render Image [" << src << "]" << std::endl;
    NetImageElement* img = new NetImageElement(
        src,
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

void BrocContainer::get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz) {
    printf("Requested image size for: %s\n", src);
}

void BrocContainer::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg) {
    printf("Requested to draw background\n");
}

void BrocContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) {

    CST_Rect dimens = {
        draw_pos.left(),
        draw_pos.top(),
        draw_pos.right() - draw_pos.left(),
        draw_pos.bottom() - draw_pos.top()
    };

    // printf("Border drawn at: %d, %d, %d, %d\n", dimens.x, dimens.y, dimens.w, dimens.h);

    auto renderer = RootDisplay::mainDisplay->renderer;
    CST_SetDrawColorRGBA(renderer, 0, 0, 0, 255);
    CST_DrawRect(renderer, &dimens);
}

void BrocContainer::set_caption(const litehtml::tchar_t* caption ) {
    // std::cout << "Setting caption to: " << caption << std::endl;
    CST_SetWindowTitle(caption);
}

void BrocContainer::set_base_url(const litehtml::tchar_t* base_url ) {
    // std::cout << "Setting base url to: " << base_url << std::endl;
    this->base_url = base_url;
}

void BrocContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el ) {
    auto href = el->get_attr("href");
    auto rel = el->get_attr("rel");
    std::cout << "<link> tag detected for href: " << href << " with rel: " << rel << std::endl;
}

void BrocContainer::on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el ) {
    std::cout << "Anchor clicked: " << url << std::endl;
}

void BrocContainer::set_cursor(const litehtml::tchar_t* cursor ) {
    std::cout << "Setting cursor to: " << cursor << std::endl;
}

void BrocContainer::transform_text(litehtml::tstring& text, litehtml::text_transform tt ) {
    std::cout << "Transforming text: " << text << std::endl;
}

void BrocContainer::import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl ) {
    std::cout << "Importing CSS: " << text << ", " << url << std::endl;
}

void BrocContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y ) {
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

std::shared_ptr<litehtml::element> BrocContainer::create_element(const litehtml::tchar_t *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc) {
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

void BrocContainer::get_language(litehtml::tstring& language, litehtml::tstring & culture ) const {
    printf("Getting language\n");
}
