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
        std::copy(buffer.pixels(), buffer.pixels() + (buffer.getWidth() * buffer.getHeight()), frameBuffer);
    }
}
#endif