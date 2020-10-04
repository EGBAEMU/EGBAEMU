#ifndef FB_CANVAS_HPP
#define FB_CANVAS_HPP

#include <lcd/defs.hpp>
#include <lcd/canvas.hpp>


namespace gbaemu::lcd
{
    typedef uint16_t fbcolor_t;

    class FBCanvas : public Canvas<fbcolor_t>
    {
      private:
        int device;
        size_t size;
        fbcolor_t *frameBuffer;
      public:
        FBCanvas(const char *deviceString);
        ~FBCanvas();
        virtual void beginDraw() override;
        virtual void endDraw() override;
        fbcolor_t *pixels() override;
        const fbcolor_t *pixels() const override;
    };
}

#endif /* FB_CANVAS_HPP */
