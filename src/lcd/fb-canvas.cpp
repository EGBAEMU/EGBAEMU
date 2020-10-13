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
                                                   buffer({MemoryCanvas<color_t>(SCREEN_WIDTH, SCREEN_HEIGHT), MemoryCanvas<color_t>(SCREEN_WIDTH, SCREEN_HEIGHT)}),
                                                   fbCopyThread(fbCopyLoop, std::ref(waitForFB), std::ref(*this))
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
        run = false;
        fbCopyThread.join();

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
            std::copy(canvas.buffer[lastBuf].pixels(), canvas.buffer[lastBuf].pixels() + size, canvas.frameBuffer);
        }
    }

    void FBCanvas::present()
    {
        currentBuf = (currentBuf + 1) & 1;
        waitForFB.notify_one();
    }
}
#endif