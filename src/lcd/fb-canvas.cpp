#include "fb-canvas.hpp"

#if RENDERER_USE_FB_CANVAS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


namespace gbaemu::lcd
{
    FBCanvas::FBCanvas(const char *deviceString) : Canvas(SCREEN_WIDTH, SCREEN_HEIGHT),
                                                   buffer(SCREEN_WIDTH, SCREEN_HEIGHT)
    {
        size = fbWidth * fbHeight * sizeof(color16_t);
        device = open(deviceString, O_RDWR);

        if (device < 0) {
            std::cout << "WARNING: Could not open frame buffer, present() will have no effect!" << std::endl;
            frameBuffer = nullptr;
            return;
        }

        frameBuffer = reinterpret_cast<color16_t *>(mmap(NULL, size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            device, 0));

        if (!frameBuffer)
            std::cout << "WARNING: Could not map frame buffer, present() will have no effect!" << std::endl;
    }

    FBCanvas::~FBCanvas()
    {
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
        return buffer.pixels();
    }

    const color_t *FBCanvas::pixels() const
    {
        return buffer.pixels();
    }

    void FBCanvas::present()
    {
        if (!frameBuffer)
            return;

        std::copy(buffer.pixels(), buffer.pixels() + (buffer.getWidth() * buffer.getHeight()), frameBuffer);
    }
}
#endif