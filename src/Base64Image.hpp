#pragma once

#include "../libs/chesto/src/Texture.hpp"
#include <string>

class Base64Image : public Texture
{
public:
	/// Creates a new image element by decoding the base64 string
	Base64Image(std::string base64);
};
