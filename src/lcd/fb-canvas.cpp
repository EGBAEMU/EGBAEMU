#include "fb-canvas.hpp"

#if RENDERER_USE_FB_CANVAS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


namespace gbaemu::lcd
{
    FBCanvas::FBCanvas(const char *deviceString, size_t w, size_t h) : size(w * h)
    {
        width = w;
        height = h;

        device = open(deviceString, O_RDWR);

        if (device < 0)
            throw std::runtime_error("Could not open device!");

        frameBuffer = reinterpret_cast<fbcolor_t *>(mmap(NULL, size,
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

    fbcolor_t *FBCanvas::pixels()
    {
        return frameBuffer;
    }

    const fbcolor_t *FBCanvas::pixels() const
    {
        return frameBuffer;
    }
}
#endif