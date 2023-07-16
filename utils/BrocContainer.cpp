#include "BrocContainer.hpp"
#include "../libs/chesto/src/NetImageElement.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../src/Base64Image.hpp"
#include "../src/MainDisplay.hpp"
#include "../src/URLBar.hpp"
#include "Utils.hpp"
#include "../libs/litehtml/include/litehtml/render_item.h"
#include <sstream>
#include <iostream>

// This class implements the litehtml::document_container interface.
// And hooks up litehtml methods to SDL/Chesto methods.

BrocContainer::BrocContainer(WebView* webView)
{
    this->webView = webView;
}

litehtml::uint_ptr BrocContainer::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
    // std::cout << "Requested to create font: " << faceName << std::endl;
    fm->ascent = 0;
    fm->descent = 0;

    // auto fontKey = std::string(faceName) + "_" + std::to_string(size);

    auto font = CST_CreateFont();  

    // TODO: handle system fonts based on platform (and include a few defaults here)
    // if we have a comma or spaces, grab the right-most one
    litehtml::string_vector fonts;
	litehtml::split_string(faceName, fonts, ",");
    auto lastFont = fonts[fonts.size() - 1];
	litehtml::trim(lastFont);

    std::string fontFamily = lastFont;

    auto fontPath = std::string(RAMFS "./res/fonts/");

    if (fontFamily == "serif") {
        fontPath += "PTSerif";
    } else if (fontFamily == "monospace") {
        fontPath += "UbuntuMono";
    } else if (fontFamily == "cursive") {
        fontPath += "CedarvilleCursive";
    } else if (fontFamily == "fantasy") {
        fontPath += "IndieFlower";
    } else { // sans-serif and catch-all
        fontPath += "OpenSans";
    }

    fontPath += "-";

    // determine bold/italics/both
    std::string fontSlug = "";
    int ttfStyles = 0;

    if (fontFamily != "cursive" && fontFamily != "fantasy") {
        if (weight >= 600) {
            fontSlug += "Bold";
            ttfStyles |= TTF_STYLE_BOLD;
        }
        if (italic == litehtml::font_style_italic) {
            fontSlug += "Italic";
            ttfStyles |= TTF_STYLE_ITALIC;
        }
    }

    if (fontSlug == "") {
        fontSlug = "Regular";
        ttfStyles = TTF_STYLE_NORMAL;
    }

    fontPath += fontSlug + ".ttf";

    std::cout << "Loading font: " << fontPath << std::endl;

    auto renderer = (RootDisplay::mainDisplay)->renderer;
    CST_LoadFont(font, renderer, fontPath.c_str(), size, CST_MakeColor(0,0,0,255), ttfStyles);

    // save some info about the font
    fm->x_height = CST_GetFontWidth(font, "x");
    fm->ascent = FC_GetAscent(font, "x");
    fm->descent = FC_GetDescent(font, "x");
    fm->height = FC_GetHeight(font, "x");

    // save this font to the cache
    auto fontKey = ++eternalCounter;
    this->fontCache[fontKey] = font;

    // return an ID for this font's key
    return fontKey;
}

void BrocContainer::delete_font(litehtml::uint_ptr hFont) {
    this->fontCache.erase(hFont);
}

int BrocContainer::text_width(const char* text, litehtml::uint_ptr hFont) {
    auto font = this->fontCache[hFont];
    return  CST_GetFontWidth(font, text);
}

void BrocContainer::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {
    auto renderer = RootDisplay::mainDisplay->renderer;
    auto font = this->fontCache[hFont];

    if (color.red != 0 || color.green != 0 || color.blue != 0) {
        // color font! create an effect and use that to draw
        auto align = FC_ALIGN_LEFT;
        auto effect = FC_MakeEffect(align, FC_MakeScale(1, 1), CST_MakeColor(color.red, color.green, color.blue, color.alpha));
        FC_DrawEffect(font, renderer, pos.left(), pos.top(), effect, text);
    } else {
        CST_DrawFont(font, renderer, pos.left(), pos.top(), text);
    }
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

    printf("Going to resolve url: %s\n", src);

    if (
        strncmp(src, "https://", 8) == 0
        || strncmp(src, "http://", 7) == 0
        || strncmp(src, "data:", 5) == 0
        || strncmp(src, "mailto:", 7) == 0
        || strncmp(src, "javascript:", 11) == 0
        || strncmp(src, "special:", 8) == 0
        || strncmp(src, "localhost", 9) == 0
        || strncmp(src, "file://", 7) == 0
    ) {
        // this is a https/http/data/mailto/js uri, just use it directly
        ss << src;
        url = ss.str();
    }
    else if (baseurl == 0 || *baseurl == '\0') {
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
            ss << this->base_url;
            // make sure there's a trailing slash
            if (this->base_url[this->base_url.length() - 1] != '/') {
                ss << "/";
            }
            ss << src;
        }

        url = ss.str();
    }
    printf("Resolved url: %s\n", url.c_str());

    return url;
}

void BrocContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready) {

    std::string newUrl = resolve_url(src, baseurl);
    Texture* img = nullptr;
    
    std::string srcString = src;

    // if it starts with "data:", it's a data url, so we can just load it directly
    if (srcString.substr(0, 5) == "data:") {
        // TODO: re-use this datauri logic, for other non-image mimetypes
        auto isBase64 = srcString.find(";base64") != std::string::npos;
        auto data = srcString.substr(srcString.find(",") + 1);

        if (!isBase64) {
            return; // unsupported (what would this mean?)
        }

        // decode the base64 and load the iamge
        img = new Base64Image(data);

    } else if (srcString.substr(0, 7) == "file://") {
        // local file, load it from the filesystem
        auto urlCopy = new std::string(newUrl.c_str());
        // file URLs are loaded relatively from the data directory (TODO: absolute paths? security implications?)
        // this is technically in violation of the file:// uri scheme
        img = new ImageElement(("./data/" + urlCopy->substr(7)).c_str());
    } else {
        // normal url, load it from the network
        // std::cout << "Requested to render Image [" << newUrl << "]" << std::endl;
        auto urlCopy = new std::string(newUrl.c_str());
        img = new NetImageElement(
            urlCopy->c_str(),
            []() {
                // could not load image, fallback
                auto fallback = new ImageElement(RAMFS "res/redx.png");
                fallback->setScaleMode(SCALE_PROPORTIONAL_WITH_BG);
                return fallback;
            }
        );
        ((NetImageElement*)img)->updateSizeAfterLoad = true;
    }

    // save this image to the map cache to be positioned later
    printf("Saving image to cache: %s\n", src);
    this->imageCache[src] = img;

    img->position(-1000, -1000); // draw off screen at first, will be positioned later (but we start loading asap)
    webView->child(img);
}

void BrocContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz) {
    // look up in cache
    auto img = this->imageCache[src];

    if (img != nullptr) {
        sz.width = img->width;
        sz.height = img->height;
    }
}

void BrocContainer::draw_background( litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bgvec ) {
    // printf("Requested to draw background\n");
    auto renderer = RootDisplay::mainDisplay->renderer;

    for (auto bg : bgvec) {
        CST_Rect dimens = {
            bg.clip_box.x,
            bg.clip_box.y,
            bg.clip_box.width,
            bg.clip_box.height
        };

        // printf("Background drawn at: %d, %d, %d, %d\n", dimens.x, dimens.y, dimens.w, dimens.h);
        // printf("Name of image: %s\n", bg.image.c_str());

        // position the image according to the background position
        if (bg.image.length() > 0) {
            auto img = this->imageCache[bg.image];
            if (img != nullptr) {
                // printf("Positioning image: %s, postion %d, %d\n", bg.image.c_str(), bg.position_x, bg.position_y);
                img->setPosition(webView->x + bg.position_x, -1*webView->y + bg.position_y);
                img->setSize(dimens.w, dimens.h);

                // if it's not a background attachment, move it to the front
                // if (bg.attachment ) {
                    // printf("Moving image named %s to front, background_attachment=%d\n", bg.image.c_str(), bg.attachment);
                    // img->moveToFront();
                // }
            }
        } else {
            // no image, draw a filled rectangle
            auto radius = bg.border_radius.top_left_x; // TODO: all corners?
            CST_roundedBoxRGBA(renderer, dimens.x, dimens.y, dimens.x + dimens.w, dimens.y + dimens.h, radius, bg.color.red, bg.color.green, bg.color.blue, bg.color.alpha);
        }
    }

}

void BrocContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) {

    if (!borders.is_visible()) {
        // don't draw invisible borders!
        return;
    }

    CST_Rect dimens = {
        draw_pos.left(),
        draw_pos.top(),
        draw_pos.right() - draw_pos.left(),
        draw_pos.bottom() - draw_pos.top()
    };

    // printf("Border drawn at: %d, %d, %d, %d\n", dimens.x, dimens.y, dimens.w, dimens.h);

    // TODO: get other border colors, for now grab the top one
    auto color = borders.top.color;

    auto renderer = RootDisplay::mainDisplay->renderer;
    auto radius = borders.radius.top_left_x; // TODO: all corners?
    CST_roundedRectangleRGBA(renderer, dimens.x, dimens.y, dimens.x + dimens.w, dimens.y + dimens.h, radius, color.red, color.green, color.blue, color.alpha);
}

void BrocContainer::set_caption(const char* caption ) {
    // std::cout << "Setting caption to: " << caption << std::endl;
    CST_SetWindowTitle(caption);
}

void BrocContainer::set_base_url(const char* base_url ) {
    // std::cout << "Setting base url to: " << base_url << std::endl;
    std::string url = base_url;

    // we assume we have a proper protocol set
    auto protoPos = url.find("://") + 3;

    // extract base url from full URL
    auto endPos = url.find_last_of("/");
    this->base_url = url.substr(0, protoPos) + url.substr(protoPos, endPos);
    if (endPos < protoPos) {
        // just use the URL as is, with a trailing slash
        this->base_url = url;
    }

    // extract base domain from full URL
    url += "/";
    std::string protocol = url.substr(0, url.find("://"));
    std::string domain = url.substr(url.find("://") + 3);

    auto endingSlashPos = domain.find("/");
    if (endingSlashPos != std::string::npos) {
        domain = domain.substr(0, endingSlashPos);
    }
    this->base_domain = protocol + "://" + domain;

    this->protocol = protocol;
}

void BrocContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el ) {
    auto href = el->get_attr("href");
    auto rel = el->get_attr("rel");
    // std::cout << "<link> tag detected for href: " << href << " with rel: " << rel << std::endl;
}

// returns each render item's box positions, with placement offsets
std::vector<std::tuple<litehtml::position, litehtml::size>> get_draw_areas(
    std::list<std::weak_ptr<litehtml::render_item>> m_renders
)
{
	std::vector<std::tuple<litehtml::position, litehtml::size>> all_areas;
	for(const auto& ri_el : m_renders)
	{
		auto ri = ri_el.lock();
		if(ri)
		{
			litehtml::position::vector boxes;
			ri->get_inline_boxes(boxes);

			// all boxes will have their placements added to them
			auto render_pos = ri->get_placement();
			
			for(auto & box : boxes)
			{
				litehtml::position pos;
				pos.x = render_pos.x + box.x;
				pos.y = render_pos.y + box.y;

				all_areas.push_back(std::make_tuple(
					pos,
					litehtml::  size(box.width, box.height)
				));
			}
		}
	}
	return all_areas;
}

void BrocContainer::on_anchor_click(const char* url, const litehtml::element::ptr& el) {

    // use a hack to grab the internal render list from the target element
    // TODO: not do this? either fork litehtml or find another way
    auto draw_areas = get_draw_areas(el->m_renders);

    if (webView->nextLinkHref == "") {
        // copy the data from the areas in the CST_Rects of nextLinkRects
        webView->nextLinkRects.clear();
        for (auto area : draw_areas) {
            auto pos = std::get<0>(area);
            auto sz  = std::get<1>(area);
            CST_Rect rect = {
                webView->x + pos.x, webView->y + pos.y,
                sz.width, sz.height
            };
            webView->nextLinkRects.push_back(rect);
        }
        webView->nextLinkHref = url;
        return; // return early
    }
    else if (webView->nextLinkHref == url) {
        // do the actual redirect
        std::string newUrl = resolve_url(url, "");
        webView->url = newUrl;
        webView->needsLoad = true;

        auto urlBar = ((MainDisplay*)RootDisplay::mainDisplay)->urlBar;
        urlBar->urlText->setText(webView->url.c_str());
        urlBar->urlText->update();
    }
}

void BrocContainer::set_cursor(const char* cursor ) {
    // std::cout << "Setting cursor to: " << cursor << std::endl;
}

void BrocContainer::transform_text(litehtml::string& text, litehtml::text_transform tt ) {
    // std::cout << "Transforming text: " << text << std::endl;
}

void BrocContainer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl ) {
    std::cout << "Importing CSS: " << text << ", " << url << std::endl;
    std::string newUrl = resolve_url(url.c_str(), baseurl.c_str());
    std::cout << "Resolved URL: " << newUrl << std::endl;
    /// download the CSS file
    // TODO: do this asynchronously and re-update the page when it's done
    std::string contents;
    downloadFileToMemory(newUrl.c_str(), &text);
}

void BrocContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) {
    // printf("Call to set_clip with position: %d, %d, %d, %d, and border radiuses: %d, %d, %d, %d\n", pos.left(), pos.top(), pos.right(), pos.bottom(), bdr_radius.top_left_x, bdr_radius.top_left_y, bdr_radius.bottom_right_x, bdr_radius.bottom_right_y);
}

void BrocContainer::del_clip( ) {
    // printf("Call from del_clip\n");
}

void BrocContainer::get_client_rect(litehtml::position& client ) const {
    // fill client rect with viewport size
    // printf("Getting client rect\n");
    client.width = RootDisplay::screenWidth;
    client.height = RootDisplay::screenHeight;
}

std::shared_ptr<litehtml::element> BrocContainer::create_element(const char *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc) {
    // std::cout << "Requested to create an element: " << tag_name << std::endl;

    if (std::string(tag_name) == "meta" && attributes.count("name") > 0) {
        printf("Found meta tag\n");
        std::string name = attributes.at("name");
        if (name == "theme-color" && attributes.count("content") > 0) {
            printf("Found theme-color meta tag\n");
            auto content = attributes.at("content");
            // convert the hex color to a chesto color
            auto web_color = litehtml::web_color::from_string(content, NULL);
            CST_Color chesto_color = {
                web_color.red,
                web_color.green,
                web_color.blue,
                web_color.alpha
            };
            // set the theme color
            auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
            mainDisplay->theme_color = chesto_color;
            printf("Set theme color to: %d, %d, %d, %d\n", chesto_color.r, chesto_color.g, chesto_color.b, chesto_color.a);
        }
    }

    // if (strcmp(tag_name, "a") == 0) {
    //     // std::cout << "Creating an image element" << std::endl;
    //     std::cout << " Got an a tag with href attribute: " << attributes.at("href") << std::endl;
    // }

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
