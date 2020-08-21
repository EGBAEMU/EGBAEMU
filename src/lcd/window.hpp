#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL2/SDL.h>


namespace sdl {
    class Window {
      private:
        SDL_Window *window;
        SDL_Surface *surface;
      public:
        Window(uint32_t width, uint32_t height) {
            SDL_Init(SDL_INIT_VIDEO);
            window = SDL_CreateWindow("gbaemu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      width, height, SDL_WINDOW_SHOWN);
            surface = SDL_GetWindowSurface(window);
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
            SDL_UpdateWindowSurface(window);
            SDL_Delay(2000);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
    };
}

#endif /* WINDOW_HPP */
