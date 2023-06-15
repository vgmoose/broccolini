#include <sstream>
#include <iomanip>
#include "ClockElement.hpp"
#include "../utils/Utils.hpp"

ClockElement::ClockElement()
{
    this->width = 50;
    this->height = 50;

    clockFont = CST_CreateFont();
    CST_LoadFont(
        clockFont, RootDisplay::renderer,
        RAMFS "res/fonts/OpenSans-Regular.ttf",
        28, CST_MakeColor(0,0,0,255), TTF_STYLE_NORMAL
    );

    this->touchable = true;
    this->action = [this]() {
        is12Hour = !is12Hour;
    };
}

bool ClockElement::process(InputEvents* event) {
    // get the current time
    time_t now = time(0);
    tm* ltm = localtime(&now);

    // get the hour and minute
    int hour = ltm->tm_hour;
    int minute = ltm->tm_min;

    // convert to 12 hour time but only in USA, Austrailia, and English-speaking parts of canada
    // std::string locale = std::string(setlocale(LC_ALL, NULL));
    // if (locale == "en_US.UTF-8" || locale == "en_AU.UTF-8" || locale == "en_CA.UTF-8") {
    //     if (hour > 12) {
    //         hour -= 12;
    //     }
    // }
    // printf("Locale: %s\n", locale.c_str());
    // TODO: this detection is not working

    // convert to 12 hour time if the user has it enabled
    if (is12Hour && hour > 12) {
        hour -= 12;
    }

    // if the time has changed, update the values
    if (hour != topVal || minute != botVal) {
        topVal = hour;
        botVal = minute;
        return true;
    }
    
    // no update
    return Element::process(event);
}

void ClockElement::render(Element* parent) {
    auto renderer = RootDisplay::renderer;

    std::ostringstream str1;
    str1 << std::setw(2) << std::setfill('0') << topVal;
    std::string topText = str1.str();

    std::ostringstream str2;
    str2 << std::setw(2) << std::setfill('0') << botVal;
    std::string bottomText = str2.str();

    CST_DrawFont(
        clockFont, renderer,
        this->x + width/2 - 15,
        this->y + height/2 - 32,
        topText.c_str()
    );

    CST_DrawFont(
        clockFont, renderer,
        this->x + width/2 - 15,
        this->y + height/2 - 6,
        bottomText.c_str()
    );

    Element::render(parent);
}