#include "Base64Image.hpp"
#include "../utils/Utils.hpp"

Base64Image::Base64Image(std::string base64)
{
    // decode data
    std::string decoded = base64_decode(base64);
    CST_Surface *surface = IMG_Load_RW(SDL_RWFromMem((void*)decoded.c_str(), decoded.size()), 1);
    loadFromSurfaceSaveToCache(base64, surface);

    // update size!
    this->width = surface->w;
    this->height = surface->h;

    CST_FreeSurface(surface);
}
