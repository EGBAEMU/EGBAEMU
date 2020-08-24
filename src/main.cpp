#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "cpu.hpp"
#include "lcd/lcd-controller.hpp"
#include "lcd/window.hpp"

#define SHOW_WINDOW true
#define DISAS_CMD_RANGE 20

static bool run_window = true;

static void handleSignal(int signum)
{
    if (signum == SIGINT) {
        std::cout << "exiting..." << std::endl;
        run_window = false;
    }
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        std::cout << "please provide a ROM file\n";
        return 0;
    }

    /* signal and window stuff */
    std::signal(SIGINT, handleSignal);

    gbaemu::lcd::Window window(1280, 720);
    auto canv = window.getCanvas();
    canv.beginDraw();
    canv.clear(0xFFFF0000);
    canv.endDraw();
    

    /* read gba file */
    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

    std::cout << "read " << buf.size() << " bytes\n";

    gbaemu::CPU cpu;
    cpu.state.memory.loadROM(reinterpret_cast<uint8_t *>(buf.data()), buf.size());

    gbaemu::lcd::LCDisplay display(0, 0, canv);
    gbaemu::lcd::LCDController controller(display, cpu.state.memory);

    std::cout << "Game Title: ";
    for (size_t i = 0; i < 12; ++i) {
        std::cout << static_cast<char>(cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0A0 + i, nullptr));
    }
    std::cout << std::endl;
    std::cout << "Game Code: ";
    for (size_t i = 0; i < 4; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0AC + i, nullptr) << " ";
    }
    std::cout << std::endl;
    std::cout << "Maker Code: ";
    for (size_t i = 0; i < 2; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0B0 + i, nullptr) << " ";
    }
    std::cout << std::endl;

    cpu.state.accessReg(gbaemu::regs::PC_OFFSET) = gbaemu::Memory::EXT_ROM_OFFSET;
    cpu.initPipeline();

    std::cout << cpu.state.disas(gbaemu::Memory::EXT_ROM_OFFSET, DISAS_CMD_RANGE);

    for (uint32_t i = 0; i < 0xFFFFFFFF;) {
        uint32_t prevPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);

        cpu.step();

        uint32_t postPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);

        if (prevPC != postPC) {
            std::cout << "========================================================================\n";
            std::cout << cpu.state.disas(postPC, DISAS_CMD_RANGE);

            ++i;
        }

        SDL_Event event;
        
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
                break;
        }
        
        controller.plotMemory();
        window.present();

        if (!run_window)
            break;
    }

    return 0;
}
