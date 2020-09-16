#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "cpu/cpu.hpp"
#include "debugger.hpp"
#include "lcd/lcd-controller.hpp"
#include "lcd/window.hpp"

#include "input/keyboard_control.hpp"
#include "io/keypad.hpp"

#define SHOW_WINDOW true
#define DISAS_CMD_RANGE 5
#define DEBUG_STACK_PRINT_RANGE 8
#define SDL_EVENT_POLL_INTERVALL 16384

static volatile bool doRun = true;

static void handleSignal(int signum)
{
    if (signum == SIGINT) {
        std::cout << "exiting..." << std::endl;
        doRun = false;
    }
}

static void cpuLoop(gbaemu::CPU &cpu, gbaemu::lcd::LCDController &lcdController)
{

    gbaemu::debugger::ExecutionHistory history(100);

    gbaemu::debugger::Watchdog charlie;
    gbaemu::debugger::JumpTrap jumpTrap;
    charlie.registerTrap(jumpTrap);

    bool stepMode = false;
    bool preStepMode = false;
    bool dummyStepMode = false;
    //THUMB memory mirroring ROM?
    gbaemu::debugger::AddressTrap bp1(0x0800169A, &stepMode);
    gbaemu::debugger::ExecutionRegionTrap bp2(gbaemu::Memory::MemoryRegion::IWRAM, &stepMode);
    gbaemu::debugger::MemoryChangeTrap bp3(0x03007FFC, 0, &dummyStepMode);
    //gbaemu::debugger::RegisterNonZeroTrap r12trap(gbaemu::regs::R12_OFFSET, 0x08000338, &stepMode);

    charlie.registerTrap(bp1);
    charlie.registerTrap(bp2);
    charlie.registerTrap(bp3);

    std::chrono::high_resolution_clock::time_point t;

    lcdController.updateReferences();

    for (uint32_t i = 0, j = 0; doRun; ++i, ++j) {
        if (j == 1)
            t = std::chrono::high_resolution_clock::now();

        uint32_t prevPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);
        bool prevThumb = cpu.state.getFlag(gbaemu::cpsr_flags::THUMB_STATE);
        auto inst = cpu.state.pipeline.decode.instruction;

        if (cpu.step() == gbaemu::CPUExecutionInfoType::EXCEPTION) {
            std::cout << cpu.executionInfo.message << cpu.state.disas(cpu.state.accessReg(gbaemu::regs::PC_OFFSET), 32) << std::endl;
            std::cout << cpu.state.toString() << std::endl;
            std::cout << cpu.state.printStack(DEBUG_STACK_PRINT_RANGE) << std::endl;
            break;
        }

        lcdController.tick();


        uint32_t postPC = cpu.state.accessReg(gbaemu::regs::PC_OFFSET);

        if (prevPC != postPC && false) {
            history.collect(&cpu, prevPC);
            charlie.check(prevPC, postPC, prevThumb ? cpu.thumbDecoder.decode(inst) : cpu.armDecoder.decode(inst), cpu.state);

            if (stepMode) {
                if (stepMode != preStepMode) {
                    std::cout << "=========== Execution History ==========" << std::endl;
                    history.dumpHistory(&cpu);
                    std::cout << "========================================" << std::endl;
                }

                std::cout << "IRQ Handler: 0x" << std::hex << cpu.state.memory.read32(0x03007FFC, nullptr, false, true) << std::endl;
                std::cout << cpu.state.disas(cpu.state.memory.read32(0x03007FFC, nullptr, false, true), DISAS_CMD_RANGE) << std::endl;
                std::cout << "press enter to continue\n";
                std::cin.get();

                std::cout << "========================================================================\n";
                std::cout << cpu.state.disas(postPC, DISAS_CMD_RANGE);
                std::cout << cpu.state.toString() << '\n';
                std::cout << cpu.state.printStack(DEBUG_STACK_PRINT_RANGE) << '\n';
                preStepMode = stepMode;
            }
        }

        if (j >= 1001) {
            double dt = std::chrono::duration_cast<std::chrono::microseconds>((std::chrono::high_resolution_clock::now() - t)).count();
            /*
                dt = us * 1000
                us for a single instruction = dt / 1000

            */

            double mhz = (1000000 / (dt / 1000)) / 1000000;

            /*
            if (mhz < 16)
                std::cout << std::dec << dt << "us for 1000 cycles => ~" << mhz << "MHz" << std::endl;
             */

            j = 0;
        }
    }

    doRun = false;
    // std::cout << jumpTrap.toString() << std::endl;
}

int main(int argc, char **argv)
{
    if (argc <= 1) {
        std::cout << "please provide a ROM file\n";
        return 0;
    }

    /* read gba file */
    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

    /* signal and window stuff */
    std::signal(SIGINT, handleSignal);

    gbaemu::lcd::Window window(1280, 720);
    auto canv = window.getCanvas();
    canv.beginDraw();
    canv.clear(0xFF365e7a);
    canv.endDraw();
    window.present();
    gbaemu::lcd::WindowCanvas windowCanvas = window.getCanvas();

    /* intialize CPU and print game info */
    gbaemu::CPU cpu;
    cpu.state.memory.loadROM(reinterpret_cast<uint8_t *>(buf.data()), buf.size());

    /* initialize SDL and LCD */
    std::mutex canDrawToScreenMutex;
    bool canDrawToScreen = false;
    gbaemu::lcd::LCDController controller(windowCanvas, &cpu, &canDrawToScreenMutex, &canDrawToScreen);

    std::cout << "Game Title: ";
    for (uint32_t i = 0; i < 12; ++i) {
        std::cout << static_cast<char>(cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0A0 + i, nullptr));
    }
    std::cout << std::endl;
    std::cout << "Game Code: ";
    for (uint32_t i = 0; i < 4; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0AC + i, nullptr) << " ";
    }
    std::cout << std::endl;
    std::cout << "Maker Code: ";
    for (uint32_t i = 0; i < 2; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0B0 + i, nullptr) << " ";
    }
    std::cout << std::endl;

    cpu.state.accessReg(gbaemu::regs::PC_OFFSET) = gbaemu::Memory::EXT_ROM_OFFSET;
    cpu.initPipeline();

    std::cout << "Max legit ROM address: 0x" << std::hex << (gbaemu::Memory::EXT_ROM_OFFSET + cpu.state.memory.getRomSize() - 1) << std::endl;

    gbaemu::Keypad keypad(&cpu);
    gbaemu::keyboard::KeyboardController gameController(keypad);

    std::cout << "INFO: Launching CPU thread" << std::endl;
    std::thread cpuThread(cpuLoop, std::ref(cpu), std::ref(controller));

    while (doRun) {
        SDL_Event event;

        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
                break;

            gameController.processSDLEvent(event);
        }

        if (canDrawToScreenMutex.try_lock()) {
            if (canDrawToScreen) {
                window.present();
            }

            canDrawToScreen = false;
            canDrawToScreenMutex.unlock();
        }
    }

    doRun = false;
    std::cout << "window closed" << std::endl;

    /* kill LCDController thread and wait */
    controller.exitThread();
    /* wait for cpu thread to exit */
    cpuThread.join();

    return 0;
}
