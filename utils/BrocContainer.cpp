#include "BrocContainer.hpp"
#include "../libs/chesto/src/NetImageElement.hpp"
#include "../libs/chesto/src/ImageElement.hpp"
#include "../src/JSEngine.hpp"
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

litehtml::uint_ptr BrocContainer::create_font(const litehtml::font_description& descr, const litehtml::document* doc, litehtml::font_metrics* fm) {
    // std::cout << "Requested to create font: " << faceName << ", size: " << size << ", weight: " << weight << ", italic: " << italic << ", decoration: " << decoration << std::endl;
    fm->ascent = 0;
    fm->descent = 0;

    auto faceName = descr.family;
    auto size = descr.size;

    auto italic = descr.style;
    auto weight = descr.weight;    

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

litehtml::pixel_t BrocContainer::text_width(const char* text, litehtml::uint_ptr hFont) {
    auto font = this->fontCache[hFont];
    return (litehtml::pixel_t)CST_GetFontWidth(font, text);
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

litehtml::pixel_t BrocContainer::pt_to_px(float pt) const {
    return (litehtml::pixel_t)pt;
}

litehtml::pixel_t BrocContainer::get_default_font_size() const {
    return 20.0f;
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

    // printf("Going to resolve url: %s\n", src);

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
            std::string curUrl = this->base_url;
            // strip out the GET parameters (TODO: does this affect the baseurl elsewhere?)
            auto questionMark = curUrl.find("?");
            if (questionMark != std::string::npos) {
                curUrl = curUrl.substr(0, questionMark);
            }
            // same for the hash
            auto hash = curUrl.find("#");
            if (hash != std::string::npos) {
                curUrl = curUrl.substr(0, hash);
            }
            // make sure there's a trailing slash
            if (curUrl[curUrl.length() - 1] != '/') {
                curUrl += "/";
            }
            ss << curUrl << src;
        }

        url = ss.str();
    }
    // printf("Resolved url: %s\n", url.c_str());

    return url;
}

void BrocContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready) {
    printf("We got a request to render an image\n");
    std::string newUrl = resolve_url(src, baseurl);
    Texture* img = nullptr;

    // if we already have this image in the cache, don't load it again
    if (this->imageCache.find(newUrl) != this->imageCache.end()) {
        printf("We already have this image in the cache: %s\n", newUrl.c_str());
        return;
    }
    
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
        // printf("LOAD to render Image [%s]\n", urlCopy->c_str());
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
    printf("Saving image to memory cache: %s\n", newUrl.c_str());
    this->imageCache[newUrl] = img;

    img->position(-1000, -1000); // draw off screen at first, will be positioned later 
    // img->moveToFront();
    webView->child(img);
}

void BrocContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz) {
    // look up in cache
    auto resolvedUrl = resolve_url(src, baseurl);
    auto img = this->imageCache[resolvedUrl];

    if (img != nullptr) {
        sz.width = img->width;
        sz.height = img->height;
    }
}

void BrocContainer::draw_image(
    litehtml::uint_ptr hdc,
    const litehtml::background_layer& bg,
    const std::string& url,
    const std::string& base_url
) {
    // printf("Requested to draw image\n");
    auto renderer = RootDisplay::mainDisplay->renderer;

    CST_Rect dimens = {
        bg.clip_box.x,
        bg.clip_box.y,
        bg.clip_box.width,
        bg.clip_box.height
    };

    // printf("Background drawn at: %d, %d, %d, %d\n", dimens.x, dimens.y, dimens.w, dimens.h);
    // printf("Name of image: %s\n", url.c_str());

    // position the image according to the background position
    if (url.length() > 0) {
        auto resolvedUrl = resolve_url(url.c_str(), base_url.c_str());
        auto img = this->imageCache[resolvedUrl];
        if (img != nullptr) {
            // printf("Positioning image: %s, postion %f, %f\n", url.c_str(), bg.clip_box.x, bg.clip_box.y);
            img->setPosition(webView->x + bg.clip_box.x, -1*webView->y + bg.clip_box.y);
            img->setSize(dimens.w, dimens.h);

            // if it's not a background attachment, move it to the front
            // if (bg.attachment ) {
                // printf("Moving image named %s to front, background_attachment=%d\n", bg.image.c_str(), bg.attachment);
                // img->moveToFront();
            // }
        }
    } else {
        // no image, draw a filled rectangle
        printf("NOTICE: Shouldn't be here, draw_solid_fill should have been called instead\n");
        auto radius = bg.border_radius.top_left_x; // TODO: all corners?
        // CST_roundedBoxRGBA(renderer, dimens.x, dimens.y, dimens.x + dimens.w, dimens.y + dimens.h, radius, bg.color.red, bg.color.green, bg.color.blue, bg.color.alpha);
    }
}

void BrocContainer::draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) {
    // printf("Requested to draw solid fill\n");
    auto renderer = RootDisplay::mainDisplay->renderer;

    CST_Rect dimens = {
        layer.border_box.x, // clip_box?
        layer.border_box.y,
        layer.border_box.width,
        layer.border_box.height
    };

    // printf("Fill drawn at: %d, %d, %d, %d\n", dimens.x, dimens.y, dimens.w, dimens.h);
    // printf("Color: %d, %d, %d, %d\n", color.red, color.green, color.blue, color.alpha);

    // auto chestoColor = CST_MakeColor(color.red, color.green, color.blue, color.alpha);
    CST_roundedBoxRGBA(
        renderer, dimens.x, dimens.y, dimens.x + dimens.w, dimens.y + dimens.h,
        layer.border_radius.top_left_x, // harcoded to top left corner for now
        color.red, color.green, color.blue, color.alpha
    );

}

void BrocContainer::draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::linear_gradient& gradient) {
    printf("TODO: Requested to draw linear gradient\n");
}

void BrocContainer::draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::radial_gradient& gradient) {
    printf("TODO: Requested to draw radial gradient\n");
}

void BrocContainer::draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::conic_gradient& gradient) {
    printf("TODO: Requested to draw conic gradient\n");
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
    // Only set title from HTML if it hasn't been set dynamically via JavaScript
    if (!webView->titleSetDynamically) {
        webView->setTitle(caption);
        webView->titleSetDynamically = false; // Reset flag since this was from HTML parsing
    }
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
    // TODO: get_attr can return unsafe values, so we should check for existence
    // auto href = el->get_attr("href");
    // auto rel = el->get_attr("rel");
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
    // This method is now disabled - link clicks are handled by invisible Element overlays
    // See createChestoLinksFromHTML() and handleLinkClick() for the new implementation
    std::cout << "on_anchor_click called but disabled - using invisible overlays instead" << std::endl;
}

void BrocContainer::set_cursor(const char* cursor ) {
    std::string cursorStr = cursor;

    // auto el = m_root_render->get_element_by_point(0, 0, 10, 10);
    // printf("ELEMENT: %s\n", el->get_tagName().c_str());

    if (cursorStr == "pointer") {
        CST_SetCursor(CST_CURSOR_HAND);
    } else {
        CST_SetCursor(CST_CURSOR_ARROW);
    }
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

void BrocContainer::get_viewport(litehtml::position& client ) const {
    // fill client rect with viewport size
    // printf("Getting client rect\n");
    client.width = RootDisplay::screenWidth * webView->zoomLevel;
    client.height = RootDisplay::screenHeight * webView->zoomLevel;
}

std::shared_ptr<litehtml::element> BrocContainer::create_element(const char *tag_name, const litehtml::string_map &attributes, const std::shared_ptr<litehtml::document> &doc) {
    // std::cout << "Requested to create an element: " << tag_name << std::endl;

    if (std::string(tag_name) == "button") {
        std::cout << "Creating button element" << std::endl;
        
        // We'll let litehtml create the default button element
        // but we'll also track it for later when we need to create the Chesto button overlay
        auto element = std::shared_ptr<litehtml::element>(nullptr); // Let litehtml create the default
        
        return element; // Return nullptr to use default litehtml button element
    }

    if (std::string(tag_name) == "meta" && attributes.count("name") > 0) {
        printf("Found meta tag\n");
        std::string name = attributes.at("name");
        if (name == "theme-color" && attributes.count("content") > 0) {
            printf("Found theme-color meta tag\n");
            auto content = attributes.at("content");
            // convert the hex color to a chesto color
            litehtml::web_color web_color;
            // try to parse it as a HEX, IDENT, FUNCTION, or STRING
            std::vector<litehtml::css_token_type> types = {
                litehtml::css_token_type::HASH,
                litehtml::css_token_type::IDENT,
                litehtml::css_token_type::FUNCTION,
                litehtml::css_token_type::STRING
            };
            // if it starts with a #, remove it
            if (content[0] == '#') {
                content = content.substr(1);
            }
            // try to parse as each type
            for (auto type : types) {
                litehtml::css_token cssToken(litehtml::css_token_type(type), content.c_str());
                bool parsed = litehtml::parse_color(cssToken, web_color, this);
                if (parsed) {
                    CST_Color chesto_color = {
                        web_color.red,
                        web_color.green,
                        web_color.blue,
                        web_color.alpha
                    };
                    // set the theme color
                    auto mainDisplay = (MainDisplay*)RootDisplay::mainDisplay;
                    webView->theme_color = chesto_color;
                    printf("Set theme color to: %d, %d, %d, %d\n", chesto_color.r, chesto_color.g, chesto_color.b, chesto_color.a);
                    // also set the entire background of the window to this color
                    mainDisplay->backgroundColor = fromRGB(chesto_color.r, chesto_color.g, chesto_color.b);
                    mainDisplay->hasBackground = true;
                    break;
                }
            }
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
    media.type = litehtml::media_type_screen;
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

void BrocContainer::on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) {
    // Mouse events are now handled by Chesto Button widgets instead
    // printf("Mouse event: %d\n", event);
}

void BrocContainer::handleButtonClick(const litehtml::element::ptr& button_element) {
    if (!button_element) return;
    
    std::cout << "Processing button click..." << std::endl;
    
    // Execute any JavaScript onclick handler
    executeJavaScriptOnClick(button_element);
}

void BrocContainer::executeJavaScriptOnClick(const litehtml::element::ptr& element) {
    if (!element || !webView || !webView->jsEngine) return;
    
    // Get the onclick attribute
    const char* onclick = element->get_attr("onclick");
    if (onclick && strlen(onclick) > 0) {
        std::cout << "Executing onclick JavaScript: " << onclick << std::endl;
        
        // Execute the JavaScript code
        bool success = webView->jsEngine->executeScript(onclick);
        if (!success) {
            std::cout << "Failed to execute onclick JavaScript" << std::endl;
        }
    } else {
        std::cout << "No onclick handler found for button" << std::endl;
    }
}

void BrocContainer::cleanupChestoButtons() {
    std::cout << "Starting cleanup of button overlays..." << std::endl;
    
    // Just remove overlays from webView's children list and clear registry
    // Don't delete the Elements - let Chesto handle cleanup automatically
    for (auto& pair : buttonRegistry) {
        Element* overlay = pair.second;
        if (overlay) {
            std::cout << "Removing button overlay from webView..." << std::endl;
            // Remove overlay from webView's children list
            webView->remove(overlay);
            // Note: Not deleting the overlay - Chesto will clean it up automatically
        }
    }
    buttonRegistry.clear();
    chestoButtonsCreated = false; // Reset the flag
    std::cout << "Button overlays removed from webView (Chesto will handle cleanup)" << std::endl;
}

void BrocContainer::cleanupChestoLinks() {
    std::cout << "Starting cleanup of link overlays..." << std::endl;
    
    // Just remove overlays from webView's children list and clear registry
    // Don't delete the Elements - let Chesto handle cleanup automatically
    for (auto& pair : linkRegistry) {
        Element* overlay = pair.second;
        if (overlay) {
            std::cout << "Removing link overlay from webView..." << std::endl;
            // Remove overlay from webView's children list
            webView->remove(overlay);
            // Note: Not deleting the overlay - Chesto will clean it up automatically
        }
    }
    linkRegistry.clear();
    chestoLinksCreated = false; // Reset the flag
    std::cout << "Link overlays removed from webView (Chesto will handle cleanup)" << std::endl;
}

void BrocContainer::cleanupAllOverlays() {
    cleanupChestoButtons();
    cleanupChestoLinks();
}

void BrocContainer::createChestoButtonsFromHTML() {
    if (!webView || !webView->m_doc || chestoButtonsCreated || navigationInProgress) {
        return; // Skip if already created, no document, or navigation in progress
    }
    
    std::cout << "Creating Chesto buttons from HTML button elements..." << std::endl;
    
    // Find all button elements in the document
    auto root = webView->m_doc->root();
    if (!root) {
        std::cout << "No root element found" << std::endl;
        return;
    }
    
    // Use litehtml's select_all method to find all button elements
    litehtml::elements_list button_elements = root->select_all("button");
    
    std::cout << "Found " << button_elements.size() << " button elements" << std::endl;
    
    bool createdAnyButtons = false;
    for (auto& html_button : button_elements) {
        if (createChestoButtonFromElement(html_button)) {
            createdAnyButtons = true;
        }
    }
    
    if (createdAnyButtons) {
        chestoButtonsCreated = true;
        std::cout << "Chesto buttons created successfully" << std::endl;
    }
}

void BrocContainer::createChestoLinksFromHTML() {
    if (!webView || !webView->m_doc || chestoLinksCreated || navigationInProgress) {
        return; // Skip if already created, no document, or navigation in progress
    }
    
    std::cout << "Creating Chesto links from HTML anchor elements..." << std::endl;
    
    // Find all link elements in the document
    auto root = webView->m_doc->root();
    if (!root) {
        std::cout << "No root element found for links" << std::endl;
        return;
    }
    
    // Use litehtml's select_all method to find all anchor elements with href
    litehtml::elements_list link_elements = root->select_all("a[href]");
    
    std::cout << "Found " << link_elements.size() << " link elements" << std::endl;
    
    bool createdAnyLinks = false;
    for (auto& html_link : link_elements) {
        if (createChestoLinkFromElement(html_link)) {
            createdAnyLinks = true;
        }
    }
    
    if (createdAnyLinks) {
        chestoLinksCreated = true;
        std::cout << "Chesto links created successfully" << std::endl;
    }
}

bool BrocContainer::createChestoButtonFromElement(const litehtml::element::ptr& html_button) {
    if (!html_button) return false;
    
    // Get button text content for debugging
    std::string buttonText;
    html_button->get_text(buttonText);
    if (buttonText.empty()) {
        buttonText = "Button"; // Default text
    }
    
    std::cout << "Creating invisible overlay for button: '" << buttonText << "'" << std::endl;
    
    // Get the button's position using get_placement()
    litehtml::position placement = html_button->get_placement();
    
    if (placement.width <= 0 || placement.height <= 0) {
        std::cout << "Button has no dimensions (w:" << placement.width << " h:" << placement.height << "), skipping" << std::endl;
        return false;
    }
    
    std::cout << "Button placement: x=" << placement.x << " y=" << placement.y 
              << " w=" << placement.width << " h=" << placement.height << std::endl;
    
    // Create invisible Element overlay 
    Element* overlay = new Element();
    
    // Set dimensions and make it touchable but invisible
    overlay->width = placement.width;
    overlay->height = placement.height;
    overlay->touchable = true;
    overlay->hidden = false;  // Not hidden, just no background
    overlay->hasBackground = false;  // No background = invisible
    
    // Position the overlay over the HTML button
    // Note: WebView is already positioned at y = urlBar->height, so we don't add offset
    overlay->position(placement.x, placement.y);
    
    // Set up the click callback
    overlay->action = [this, html_button]() {
        std::cout << "Invisible overlay clicked for button!" << std::endl;
        this->executeJavaScriptOnClick(html_button);
    };
    
    // Add overlay to webView as a child
    webView->child(overlay);
    
    // Store in registry for cleanup later
    buttonRegistry[html_button] = overlay;
    
    std::cout << "Created invisible overlay at (" << placement.x << ", " << placement.y 
              << ") with size " << placement.width << "x" << placement.height << std::endl;
    
    return true;
}

bool BrocContainer::createChestoLinkFromElement(const litehtml::element::ptr& html_link) {
    // TODO: copypasta'd from Button creation above, very similar / consolidate
    if (!html_link) return false;
    
    // Get the href attribute
    const char* href = html_link->get_attr("href");
    if (!href || strlen(href) == 0) {
        std::cout << "Link has no href attribute, skipping" << std::endl;
        return false;
    }
    
    // Get link text content for debugging
    std::string linkText;
    html_link->get_text(linkText);
    if (linkText.empty()) {
        linkText = href; // Use href as fallback
    }
    
    std::cout << "Creating invisible overlay for link: '" << linkText << "' -> " << href << std::endl;
    
    // create multiple buttons surrounding the render areas for this element
    auto draw_areas = get_draw_areas(html_link->m_renders);

    // if the position/size info is zero, use direct render data instead (why?)
    auto placement = html_link->get_placement();
    if (placement.width > 0 && placement.height > 0) {
        Element* overlay = new Element();
        overlay->width = placement.width;
        overlay->height = placement.height;
        overlay->touchable = true;
        overlay->hidden = false;  // Not hidden, just no background
        overlay->hasBackground = false;  // No background = invisible
        
        // Position the overlay over the HTML link
        overlay->position(placement.x, placement.y);

        // Set up the click callback
        overlay->action = [this, html_link]() {
            std::cout << "Invisible overlay clicked for link!" << std::endl;
            this->handleLinkClick(html_link);
        };
        
        // Add overlay to webView as a child
        webView->child(overlay);
        
        // Store in registry for cleanup later
        linkRegistry[html_link] = overlay;
        return true;
    }

    // continuing to use old render method (still uses invisible overlays)

    for (auto area : draw_areas) {
        auto pos = std::get<0>(area);
        auto sz  = std::get<1>(area);

        auto pos_x = webView->x + pos.x;
        auto pos_y = webView->y + pos.y;

        // Create invisible Element overlay 
        // TODO: was copypasta'd from above (need a generic one line way to make quick overlays)
        Element* overlay = new Element();
        overlay->width = sz.width;
        overlay->height = sz.height;
        overlay->touchable = true;
        overlay->hidden = false;  // Not hidden, just no background
        overlay->hasBackground = false;  // No background = invisible
        
        // Position the overlay over the HTML link
        overlay->position(pos.x, pos.y);

        // Set up the click callback
        overlay->action = [this, html_link]() {
            std::cout << "Invisible overlay clicked for link!" << std::endl;
            this->handleLinkClick(html_link);
        };
        
        // Add overlay to webView as a child
        webView->child(overlay);
        
        // Store in registry for cleanup later
        linkRegistry[html_link] = overlay;

        std::cout << "Created invisible link overlay at (" << pos.x << ", " << pos.y
                  << ") with size " << sz.width << "x" << sz.height << std::endl;
    }
    
    return true;
}

void BrocContainer::handleLinkClick(const litehtml::element::ptr& link_element) {
    if (!link_element) return;
    
    std::cout << "Processing link click..." << std::endl;
    
    // Get the href attribute
    const char* href = link_element->get_attr("href");
    if (!href || strlen(href) == 0) {
        std::cout << "Link has no href attribute" << std::endl;
        return;
    }
    
    std::cout << "Navigating to: " << href << std::endl;
    
    // Resolve the URL and navigate (using the same logic as the old on_anchor_click)
    std::string newUrl = resolve_url(href, "");
    webView->url = newUrl;
    webView->needsLoad = true;

    // Update URL bar
    auto urlBar = ((MainDisplay*)RootDisplay::mainDisplay)->urlBar;
    urlBar->urlText->setText(webView->url.c_str());
    urlBar->urlText->update();
}
    