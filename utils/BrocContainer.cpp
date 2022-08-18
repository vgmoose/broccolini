#include "BrocContainer.hpp"

// This class implements the litehtml::document_container interface.
// And hooks up litehtml methods to SDL/Chesto methods.

BrocContainer::BrocContainer()
{
}

litehtml::uint_ptr BrocContainer::create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
    return 0;
}

void BrocContainer::delete_font(litehtml::uint_ptr hFont) {

}

int BrocContainer::text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont) {
    return 0;
}

void BrocContainer::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {

}

int BrocContainer::pt_to_px(int pt) const {
    return pt;
}

int BrocContainer::get_default_font_size() const {
    return 14;
}

const litehtml::tchar_t* BrocContainer::get_default_font_name() const {
    return "OpenSans";
}

void BrocContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) {

}

void BrocContainer::load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready) {

}

void BrocContainer::get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz) {

}

void BrocContainer::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg) {

}

void BrocContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) {

}

void BrocContainer::set_caption(const litehtml::tchar_t* caption ) {

}

void BrocContainer::set_base_url(const litehtml::tchar_t* base_url ) {

}

void BrocContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el ) {

}

void BrocContainer::on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el ) {

}

void BrocContainer::set_cursor(const litehtml::tchar_t* cursor ) {

}

void BrocContainer::transform_text(litehtml::tstring& text, litehtml::text_transform tt ) {

}

void BrocContainer::import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl ) {

}

void BrocContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y ) {

}

void BrocContainer::del_clip( ) {

}

void BrocContainer::get_client_rect(litehtml::position& client ) const {

}

std::shared_ptr<litehtml::element> BrocContainer::create_element(const litehtml::tchar_t *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc) {
    return 0;
}

void BrocContainer::get_media_features(litehtml::media_features& media ) const {

}

void BrocContainer::get_language(litehtml::tstring& language, litehtml::tstring & culture ) const {

}
