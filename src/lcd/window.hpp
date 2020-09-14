#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "windowCanvas.hpp"

namespace gbaemu::lcd
{
    class Window
    {
      private:
        SDL_Window *window;
        SDL_Surface *surface;

      public:
        Window(uint32_t width, uint32_t height, const char *title = "gbaemu");
        ~Window();
        void present();
        WindowCanvas getCanvas();
        WindowCanvas getCanvas() const;
    };
} // namespace gbaemu::lcd

#endif /* WINDOW_HPP */
