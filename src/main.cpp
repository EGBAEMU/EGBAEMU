#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "cpu.hpp"
#include "lcd/lcd-controller.hpp"
#include "lcd/window.hpp"
#include "debugger.hpp"

#include "input/keyboard_control.hpp"
#include "keypad.hpp"

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

    int checkPointReached = 0;
// #define TARGET_CHECKPOINT_CNT 3
// exit of helper for 0 initialization
// #define CHECKPOINT_PC 0x0800018c


#define TARGET_CHECKPOINT_CNT 1
// exit of helper for copy from ROM to RAM
#define CHECKPOINT_PC 0x080001b0

    gbaemu::debugger::Watchdog charlie;
    gbaemu::debugger::JumpTrap jumpTrap;
    
    bool stepMode = false;
    //THUMB memory mirroring ROM?
    // gbaemu::debugger::AddressTrap bp1(0x08000536, &stepMode);
    gbaemu::debugger::AddressTrap bp1(0x0800089c, &stepMode);
    //gbaemu::debugger::RegisterNonZeroTrap r12trap(gbaemu::regs::R12_OFFSET, 0x08000338, &stepMode);
    gbaemu::debugger::RegisterNonZeroTrap r12trap(gbaemu::regs::R7_OFFSET, 0x080005c2, &stepMode);

    charlie.registerTrap(jumpTrap);
    // charlie.registerTrap(bp1);
    //charlie.registerTrap(r12trap);

    gbaemu::Keypad keypad(cpu.state.memory);
    gbaemu::keyboard::KeyboardController gameController(keypad);

    for (uint32_t i = 0; i < 0xFFFFFFFF;) {
        uint32_t prevPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);
        auto inst = cpu.state.pipeline.decode.instruction;
        if (cpu.step()) {
            std::cout << "Abort execution!" << std::endl;
            break;
        }

        if (controller.tick())
            window.present();

        uint32_t postPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);

        if (prevPC != postPC) {
            charlie.check(prevPC, postPC, inst, cpu.state);

            if (postPC == CHECKPOINT_PC) {
                ++checkPointReached;
                std::cout << "CHECKPOINT REACHED: " << checkPointReached << std::endl;
            }

            if (stepMode) {
                std::cout << "press enter to continue\n";
                std::cin.get();

                std::cout << "========================================================================\n";
                std::cout << cpu.state.disas(postPC, DISAS_CMD_RANGE);
                std::cout << cpu.state.toString() << '\n';
                std::cout << cpu.state.printStack(DEBUG_STACK_PRINT_RANGE) << '\n';
            }

            ++i;
        }

        //if (cpu.state.pipeline.decode.lastInstruction.isArmInstruction()) {
        if (checkPointReached >= TARGET_CHECKPOINT_CNT) {
            SDL_Event event;

            if (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
                    break;

                gameController.processSDLEvent(event);
            }

            controller.plotMemory();
            window.present();
        }

        if (!run_window) {
            std::ofstream jumpFile("jumps.txt", std::ios::out);
            jumpFile << jumpTrap.toString();
            jumpFile.close();

            break;
        }
    }

    return 0;
}
