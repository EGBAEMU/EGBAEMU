#include "fb-canvas.hpp"

#if RENDERER_USE_FB_CANVAS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>


namespace gbaemu::lcd
{
    FBCanvas::FBCanvas(const char *deviceString) : Canvas(SCREEN_WIDTH, SCREEN_HEIGHT),
                                                   buffer({MemoryCanvas<color_t>(SCREEN_WIDTH, SCREEN_HEIGHT), MemoryCanvas<color_t>(SCREEN_WIDTH, SCREEN_HEIGHT)}),
                                                   fbCopyThread(fbCopyLoop, std::ref(waitForFB), std::ref(*this))
    {
        device = open(deviceString, O_RDWR);

        if (device < 0) {
            std::cout << "WARNING: Could not open frame buffer, present() will have no effect!" << std::endl;
            frameBuffer = nullptr;
            return;
        }

        fb_var_screeninfo screeninfo;
        ioctl(device, FBIOGET_VSCREENINFO, &screeninfo);

        std::cout << "INFO: Framebuffer opened\n" <<
            "INFO: Screen resolution is " << std::dec << screeninfo.xres << 'x' << screeninfo.yres << std::endl;

        if (screeninfo.xres != fbWidth || screeninfo.yres != fbHeight)
            throw std::runtime_error("Unsupported screen resolution");

        size = fbWidth * fbHeight * sizeof(color16_t);

        frameBuffer = reinterpret_cast<color16_t *>(mmap(NULL, size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            device, 0));

        if (!frameBuffer)
            std::cout << "WARNING: Could not map frame buffer, present() will have no effect!" << std::endl;
    }

    FBCanvas::~FBCanvas()
    {
        run = false;
        fbCopyThread.join();

        if (frameBuffer)
            munmap(frameBuffer, size);

        if (device >= 0)
            close(device);
    }

    void FBCanvas::beginDraw()
    {

    }

    void FBCanvas::endDraw()
    {

    }

    color_t *FBCanvas::pixels()
    {
        return buffer[currentBuf].pixels();
    }

    const color_t *FBCanvas::pixels() const
    {
        return buffer[currentBuf].pixels();
    }

    void FBCanvas::fbCopyLoop(std::condition_variable &waitForFB, const FBCanvas &canvas)
    {
        std::mutex mtx;
        std::unique_lock<std::mutex> lck(mtx);

        const size_t size = (canvas.buffer[0].getWidth() * canvas.buffer[0].getHeight());

        while(canvas.run) {
            if (waitForFB.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
                continue;
            }
            int lastBuf = (canvas.currentBuf + 1) & 1;

            const color_t *src = canvas.buffer[lastBuf].pixels();
            int32_t srcWidth = canvas.buffer[lastBuf].getWidth();

            color_t *dst = canvas.frameBuffer;
            int32_t dstWidth = canvas.fbWidth;
            int32_t dstHeight = canvas.fbHeight;

            for (int32_t y = 0; y < dstHeight; ++y) {
                int32_t srcX = srcWidth - 1 - (y * 3 / 4);

                for (int32_t x = 0; x < dstWidth; ++x) {
                    int32_t srcY = x * 2 / 3;
                    dst[y * dstWidth + x] = src[srcY * srcWidth + srcX];
                }
            }
        }
    }

    void FBCanvas::present()
    {
        currentBuf = (currentBuf + 1) & 1;
        if (currentBuf)
            waitForFB.notify_one();
    }
} // namespace gbaemu::lcd
#endif