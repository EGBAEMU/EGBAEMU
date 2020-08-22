#ifndef WINDOW_CANVAS_HPP
#define WINDOW_CANVAS_HPP

#include <SDL2/SDL.h>
#include <cassert>
#include "canvas.hpp"


namespace gbaemu::lcd {
    class WindowCanvas : public Canvas<uint32_t> {
      private:
        SDL_Surface *surface;
      public:
        typedef uint32_t PixelType;

        WindowCanvas(SDL_Surface *surf);
        virtual void beginDraw() override;
        virtual void endDraw() override;
        PixelType *pixels() override;
    };
}

#endif /* WINDOW_CANVAS_HPP */