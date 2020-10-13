#ifndef FB_CANVAS_HPP
#define FB_CANVAS_HPP

#include <lcd/defs.hpp>
#include <lcd/canvas.hpp>

#include <thread>
#include <condition_variable>
#include <mutex>

#if RENDERER_USE_FB_CANVAS
namespace gbaemu::lcd
{
    class FBCanvas : public Canvas<color_t>
    {
      private:
        MemoryCanvas<color_t> buffer[2];
        volatile int currentBuf = 0;
        /* 5 blue, 6 green, 5 reg MSB */
        int device;
        int32_t fbWidth = 240;
        int32_t fbHeight = 320;
        size_t size;
        color16_t *frameBuffer;

        std::condition_variable waitForFB;

        std::thread fbCopyThread;

        volatile bool run = true;

        static void fbCopyLoop(std::condition_variable &waitForFB, const FBCanvas& canvas);

      public:
        FBCanvas(const char *deviceString);
        ~FBCanvas();
        virtual void beginDraw() override;
        virtual void endDraw() override;
        color_t *pixels() override;
        const color_t *pixels() const override;
        void present();
    };
}
#endif

#endif /* FB_CANVAS_HPP */
