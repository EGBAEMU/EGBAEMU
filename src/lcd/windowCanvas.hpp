#ifndef WINDOW_CANVAS_HPP
#define WINDOW_CANVAS_HPP

#ifdef __clang__
/*code specific to clang compiler*/
#include <SDL2/SDL.h>
#elif __GNUC__
/*code for GNU C compiler */
#include <SDL2/SDL.h>
#elif _MSC_VER
/*usually has the version number in _MSC_VER*/
/*code specific to MSVC compiler*/
#include <SDL.h>
#elif __MINGW32__
/*code specific to mingw compilers*/
#include <SDL2/SDL.h>
#else
#error "unsupported compiler!"
#endif

#include "canvas.hpp"
#include <cassert>

namespace gbaemu::lcd
{
    class WindowCanvas : public Canvas<uint32_t>
    {
      private:
        SDL_Surface *surface;

      public:
        typedef uint32_t PixelType;

        WindowCanvas(SDL_Surface *surf);
        virtual void beginDraw() override;
        virtual void endDraw() override;
        PixelType *pixels() override;
    };
} // namespace gbaemu::lcd

#endif /* WINDOW_CANVAS_HPP */