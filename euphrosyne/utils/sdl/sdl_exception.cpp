#include "sdl_exception.hpp"

#include "SDL3/SDL_error.h"

namespace euphrosyne
{
SdlException::SdlException() : message_(format("SDL Error: {}", SDL_GetError()))
{
}

SdlException::SdlException(string_view message) : message_(format("SDL Error ({}): {}", message, SDL_GetError()))
{
}

const char *SdlException::what() const noexcept
{
    return message_.c_str();
}
} // euphrosyne