#pragma once

#include "../libs/chesto/src/Texture.hpp"
#include <string>

class ClockElement : public Element
{
public:
	ClockElement();

	CST_Font* clockFont = NULL;
	int topVal = 12;
	int botVal =  0;

	bool is12Hour = true;

	bool process(InputEvents* event);
	void render(Element* parent);
};
