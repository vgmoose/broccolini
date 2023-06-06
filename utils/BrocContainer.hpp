#include <litehtml.h>
#include "../libs/chesto/src/NetImageElement.hpp"
#include "../src/WebView.hpp"

class BrocContainer : public litehtml::document_container
{
public:
    BrocContainer(WebView* webView);
    std::string base_url; // eg. https://site.com/path/to/page
    std::string base_domain; // eg. https://site.com
    std::string protocol; // eg. https
    WebView* webView;

    int eternalCounter = 0; // used for some incremental ids

    // create a map to store all fonts on the page
    std::map<litehtml::uint_ptr, FC_Font*> fontCache;

    // create a map to store all images on the page
    std::map<std::string, NetImageElement*> imageCache;

    std::string resolve_url(const char* src, const char* baseurl);

    // overridden from the litehtml::document_container interface
    virtual litehtml::uint_ptr create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
    virtual void delete_font(litehtml::uint_ptr hFont) override;
    virtual int text_width(const char* text, litehtml::uint_ptr hFont) override;
    // virtual void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
    virtual void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
    virtual int pt_to_px(int pt) const override;
    virtual int get_default_font_size() const override;
    virtual const char* get_default_font_name() const override;
    virtual void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    virtual void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    virtual void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
    virtual void draw_background( litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bgvec ) override;
    virtual void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;
    virtual void set_caption(const char* caption ) override;
    virtual void set_base_url(const char* base_url ) override;
    virtual void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el ) override;
    virtual void on_anchor_click(const char* url, const litehtml::element::ptr& el ) override;
    virtual void set_cursor(const char* cursor ) override;
    virtual void transform_text(litehtml::string& text, litehtml::text_transform tt ) override;
    virtual void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl ) override;
    virtual void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    virtual void del_clip( ) override;
    virtual void get_client_rect(litehtml::position& client ) const override;
    virtual std::shared_ptr<litehtml::element> create_element(const char *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc) override;
    virtual void get_media_features(litehtml::media_features& media ) const override;
    virtual void get_language(litehtml::string& language, litehtml::string & culture ) const override;
};