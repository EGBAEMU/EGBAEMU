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
#include "logging.hpp"

#include "input/keyboard_control.hpp"

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

static void cpuLoop(gbaemu::CPU &cpu, gbaemu::lcd::LCDController &lcdController
#ifdef DEBUG_CLI
                    ,
                    gbaemu::debugger::DebugCLI &debugCLI
#endif
)
{

    lcdController.updateReferences();

    std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();

    for (uint32_t j = 0; doRun; ++j) {
#ifdef DEBUG_CLI
        if (debugCLI.step()) {
            break;
        }
#else
        gbaemu::CPUExecutionInfoType executionInfo = cpu.step();
        if (executionInfo != gbaemu::CPUExecutionInfoType::NORMAL) {
            std::cout << "CPU error occurred: " << cpu.executionInfo.message << std::endl;
            break;
        }
#endif
        lcdController.tick();


        if (j >= 1001) {
            double dt = std::chrono::duration_cast<std::chrono::microseconds>((std::chrono::high_resolution_clock::now() - t)).count();
            // dt = us * 1000, us for a single instruction = dt / 1000
            double mhz = (1000000 / (dt / 1000)) / 1000000;

            /*
            if (mhz < 16)
                std::cout << std::dec << dt << "us for 1000 cycles => ~" << mhz << "MHz" << std::endl;
             */

            j = 0;
            t = std::chrono::high_resolution_clock::now();
        }
    }

    doRun = false;
}

#ifdef DEBUG_CLI
static void CLILoop(gbaemu::debugger::DebugCLI &debugCLI)
{
    while (doRun) {
        std::string line;
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line == "quit")
            break;

        debugCLI.passCommand(line);
    }

    doRun = false;
}
#endif

int main(int argc, char **argv)
{
    uint16_t x = 0b11111111;
    uint16_t y = 0b111111111;
    uint16_t z = 0b100000000;
    std::cout << gbaemu::signExt<int32_t, uint16_t, 9>(x) << std::endl;
    std::cout << gbaemu::signExt<int32_t, uint16_t, 9>(y) << std::endl;
    std::cout << gbaemu::signExt<int32_t, uint16_t, 9>(z) << std::endl;

    //return 0;

    if (argc <= 1) {
        std::cout << "please provide a ROM file\n";
        return 0;
    }

    /* read gba file */
    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open ROM file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

    /* intialize CPU and print game info */
    gbaemu::CPU cpu;

    std::string saveFileName(argv[1]);
    saveFileName += ".sav";
    if (!cpu.state.memory.loadROM(saveFileName.data(), reinterpret_cast<const uint8_t *>(buf.data()), buf.size())) {
        std::cout << "could not open/create save file" << std::endl;
        return 0;
    }

    if (argc > 2) {
        std::ifstream biosFile(argv[2], std::ios::binary);

        if (!biosFile.is_open()) {
            std::cout << "could not open BIOS file\n";
            return 0;
        }

        std::vector<char> biosBuf(std::istreambuf_iterator<char>(biosFile), {});
        biosFile.close();

        std::cout << "INFO: Using external bios " << argv[2] << std::endl;
        cpu.state.memory.loadExternalBios(reinterpret_cast<const uint8_t *>(biosBuf.data()), biosBuf.size());
    }

    /* signal and window stuff */
    std::signal(SIGINT, handleSignal);

    gbaemu::lcd::Window window(1280, 720);
    auto canv = window.getCanvas();
    canv.beginDraw();
    canv.clear(0xFF365e7a);
    canv.endDraw();
    window.present();
    gbaemu::lcd::WindowCanvas windowCanvas = window.getCanvas();

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

    cpu.initPipeline();

    std::cout << "Max legit ROM address: 0x" << std::hex << (gbaemu::Memory::EXT_ROM_OFFSET + cpu.state.memory.getRomSize() - 1) << std::endl;

    gbaemu::keyboard::KeyboardController gameController(cpu.keypad);

#ifdef DEBUG_CLI
    gbaemu::debugger::DebugCLI debugCLI(cpu, controller);
#endif

    std::cout << "INFO: Launching CPU thread" << std::endl;
    std::thread cpuThread(
        cpuLoop, std::ref(cpu), std::ref(controller)
#ifdef DEBUG_CLI
                                    ,
        std::ref(debugCLI)
#endif
    );

#ifdef DEBUG_CLI
    std::cout << "INFO: Launching CLI thread" << std::endl;
    std::thread cliThread(CLILoop, std::ref(debugCLI));
#endif

    while (doRun) {
        SDL_Event event;

        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
                break;

            gameController.processSDLEvent(event);

            if (event.type == SDL_KEYDOWN) {
                static int32_t objIndex = 0;

                if (event.key.keysym.sym == SDLK_KP_PLUS) {
                    ++objIndex;
                    std::cout << "OBJ hightlight index: " << std::dec << objIndex << std::endl;
                } else if (event.key.keysym.sym == SDLK_KP_MINUS) {
                    --objIndex;
                    std::cout << "OBJ hightlight index: " << std::dec << objIndex << std::endl;
                }

                controller.objHightlightSetIndex(objIndex);
            }
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

#ifdef DEBUG_CLI
    /* When CLI is attached only quit command will exit the program! */
    cliThread.join();
#endif

    return 0;
}
