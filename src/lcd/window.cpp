#include "window.hpp"

#include <iostream>

namespace gbaemu::lcd
{
    Window::Window(uint32_t width, uint32_t height, const char *title)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow(title, 100, 100,
                                  width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        assert(window);
        surface = SDL_GetWindowSurface(window);
        assert(surface);
    }

    Window::~Window()
    {
        std::cout << "destroying window" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void Window::present()
    {
        SDL_UpdateWindowSurface(window);
    }

    WindowCanvas Window::getCanvas() const
    {
        return WindowCanvas(surface);
    }
} // namespace gbaemu::lcd
