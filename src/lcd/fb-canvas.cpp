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

        if (device < 0)
            throw std::runtime_error("Could not open device!");

        frameBuffer = reinterpret_cast<color16_t *>(mmap(NULL, size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            device, 0));

        if (!frameBuffer)
            throw std::runtime_error("Could not map device to memory!");
    }

    FBCanvas::~FBCanvas()
    {
        munmap(frameBuffer, size);
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
        const color_t *srcPixels = buffer.pixels();
        color16_t *dstPixels = frameBuffer;

        for (int32_t i = 0; i < width * height; ++i) {
            color_t srcColor = srcPixels[i];

            color_t r = (srcColor >> 16) & 0xFF;
            color_t g = (srcColor >> 8) & 0xFF;
            color_t b = srcColor & 0xFF;

            dstPixels[i] = (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
        }
    }
}
#endif