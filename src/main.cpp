#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "cpu.hpp"
#include "debugger.hpp"
#include "lcd/lcd-controller.hpp"
#include "lcd/window.hpp"

#include "input/keyboard_control.hpp"
#include "keypad.hpp"

#include "math3d.hpp"

#define SHOW_WINDOW true
#define DISAS_CMD_RANGE 5
#define DEBUG_STACK_PRINT_RANGE 5

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

    std::vector<uint32_t> sprite(64 * 64);
    for (int32_t y = 0; y < 64; ++y)
        for (int32_t x = 0; x < 64; ++x)
            sprite[y * 64 + x] = ((x % 2) == (y % 2)) ? 0xFFFFFFFF : 0xFF000000;

    common::math::real_t xScale = 5,
        yScale = 5,
        xOff = 100,
        yOff = 100,
        alpha = 0.1,
        t = 0;

    typedef common::math::real_t real_t;

    auto scaleMatrix = [](real_t x, real_t y) {
        auto result = common::math::mat<3, 3>::id();
        result[0][0] = x;
        result[1][1] = y;
        return result;
    };

    auto transMatrix = [](real_t x, real_t y) {
        auto result = common::math::mat<3, 3>::id();
        result[0][2] = x;
        result[1][2] = y;
        return result;
    };

    auto rotMatrix = [](real_t a) {
        auto result = common::math::mat<3, 3>::id();
        result[0][0] = std::cos(a);
        result[0][1] = -std::sin(a);
        result[1][0] = std::sin(a);
        result[1][1] = std::cos(a);
        return result;
    };

    while (run_window) {
        canv.beginDraw();
        canv.clear(0xFF365e7a);

        xScale = 2 + std::sin(t);
        yScale = 4 + std::cos(t);
        xOff = 50 + std::cos(t) * 70;
        alpha = t;
        t += 0.01;

        common::math::mat<3, 3> trans =
            transMatrix(xOff, yOff) *
            rotMatrix(alpha) *
            scaleMatrix(xScale, yScale);

        common::math::mat<3, 3> invTrans =
            scaleMatrix(1 / xScale, 1 / yScale) *
            rotMatrix(-alpha) *
            transMatrix(-xOff, -yOff);

        canv.drawSprite(sprite.data(), 64, 64, 64, trans, invTrans);
        
        canv.endDraw();

        window.present();
    }

    return 0;

    /* read gba file */
    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

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

    std::cout << "Max legit ROM address: 0x" << std::hex << (gbaemu::Memory::EXT_ROM_OFFSET + cpu.state.memory.getRomSize() - 1) << std::endl;

    gbaemu::debugger::Watchdog charlie;
    gbaemu::debugger::JumpTrap jumpTrap;

    bool stepMode = false;
    bool doRender = false;
    //THUMB memory mirroring ROM?
    // gbaemu::debugger::AddressTrap bp1(0x08000536, &stepMode);
    gbaemu::debugger::AddressTrap bp1(0x08000264, &stepMode);
    //gbaemu::debugger::RegisterNonZeroTrap r12trap(gbaemu::regs::R12_OFFSET, 0x08000338, &stepMode);
    gbaemu::debugger::RegisterNonZeroTrap r12trap(gbaemu::regs::R7_OFFSET, 0x080005c2, &stepMode);

    //charlie.registerTrap(jumpTrap);
    //charlie.registerTrap(bp1);
    //charlie.registerTrap(r12trap);

    gbaemu::Keypad keypad(cpu.state.memory);
    gbaemu::keyboard::KeyboardController gameController(keypad);

#define SDL_EVENT_POLL_INTERVALL 16384

    for (uint32_t i = 0;; ++i) {
        uint32_t prevPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);
        auto inst = cpu.state.pipeline.decode.instruction;
        if (cpu.step()) {
            std::cout << "Abort execution!" << std::endl;
            break;
        }

        if (controller.tick()) {
            window.present();
        }

        uint32_t postPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);

        if (prevPC != postPC) {
            charlie.check(prevPC, postPC, inst, cpu.state);

            if (stepMode) {
                std::cout << "press enter to continue\n";
                std::cin.get();

                std::cout << "========================================================================\n";
                std::cout << cpu.state.disas(postPC, DISAS_CMD_RANGE);
                std::cout << cpu.state.toString() << '\n';
                std::cout << cpu.state.printStack(DEBUG_STACK_PRINT_RANGE) << '\n';
            }
        }

        if ((i % SDL_EVENT_POLL_INTERVALL) == 0) {
            SDL_Event event;

            /*
            if (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
                    break;

                gameController.processSDLEvent(event);
            }
             */

            //controller.plotMemory();
            //window.present();
        }

        if (!run_window) {
            break;
        }
    }

    std::cout << "done" << std::endl;

    return 0;
}
