#include "windowCanvas.hpp"

namespace gbaemu::lcd
{
    WindowCanvas::WindowCanvas(SDL_Surface *surf) : surface(surf)
    {
        width = surf->w;
        height = surf->h;

        assert(surf->format->BitsPerPixel == 32);
    }

    void WindowCanvas::beginDraw()
    {
        SDL_LockSurface(surface);
    }

    void WindowCanvas::endDraw()
    {
        SDL_UnlockSurface(surface);
    }

    WindowCanvas::PixelType *WindowCanvas::pixels()
    {
        return reinterpret_cast<PixelType *>(surface->pixels);
    }
} // namespace gbaemu::lcd