#include "window.hpp"
#include "defs.hpp"
#include "logging.hpp"

#include <iostream>

namespace gbaemu::lcd
{
    Window::Window(uint32_t width, uint32_t height, const char *title) :
#if RENDERER_DECOMPOSE_LAYERS == 1
                                                                         Canvas(SCREEN_WIDTH * 3, SCREEN_HEIGHT * 4)
#else
                                                                         Canvas(SCREEN_WIDTH, SCREEN_HEIGHT)
#endif
    {
        assert(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0);
        window = SDL_CreateWindow(title, 100, 100,
                                  width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        assert(window);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        assert(renderer);
#if RENDERER_DECOMPOSE_LAYERS == 1
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH * 3, SCREEN_HEIGHT * 4);
#else
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
#if RENDERER_DECOMPOSE_LAYERS == 1
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH * 3, SCREEN_HEIGHT * 4);
        assert(texture);
        buffer = new PixelType[SCREEN_HEIGHT * SCREEN_WIDTH * 4 * 3]();
#else
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
        assert(texture);
        buffer = new PixelType[SCREEN_HEIGHT * SCREEN_WIDTH]();
#endif
    }

    Window::~Window()
    {
        LOG_LCD(std::cout << "destroying window" << std::endl;);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        delete[] buffer;
    }

    void Window::present()
    {
#if RENDERER_DECOMPOSE_LAYERS == 1
        SDL_UpdateTexture(texture, NULL, buffer, SCREEN_WIDTH * sizeof(PixelType) * 3);
#else
        SDL_UpdateTexture(texture, NULL, buffer, SCREEN_WIDTH * sizeof(PixelType));
#endif
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    void Window::beginDraw()
    {
    }

    void Window::endDraw()
    {
    }

    Window::PixelType *Window::pixels()
    {
        return buffer;
    }

    const Window::PixelType *Window::pixels() const
    {
        return buffer;
    }

} // namespace gbaemu::lcd
