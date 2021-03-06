#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "logging.hpp"

#include "cpu/cpu.hpp"
#include "debugger.hpp"
#include "input/keyboard_control.hpp"
#include "lcd/lcd-controller.hpp"

#if RENDERER_USE_FB_CANVAS == 0
#include "lcd/window.hpp"
#else
#include "lcd/fb-canvas.hpp"
#endif

#define PRINT_FPS false
#define LIMIT_FPS true

// #define DUMP_ROM

static volatile bool doRun = true;

static void handleSignal(int signum)
{
    if (signum == SIGINT) {
        std::cout << "exiting..." << std::endl;
        doRun = false;
    }
}

#ifndef LEGACY_RENDERING
static bool frame(gbaemu::CPU &cpu, gbaemu::lcd::LCDController &lcdController
#ifdef DEBUG_CLI
                  ,
                  gbaemu::debugger::DebugCLI &debugCLI
#endif
)
{

    for (int i = 0; i < 160; ++i) {
#ifndef DEBUG_CLI
        gbaemu::CPUExecutionInfoType executionInfo = cpu.step(960);
        if (executionInfo != gbaemu::CPUExecutionInfoType::NORMAL) {
            std::cout << "CPU error occurred: " << std::endl;
            std::cout << cpu.state.executionInfo.message.str() << std::endl;
            return true;
        }
#else
        for (int j = 0; j < 960; ++j) {
            if (debugCLI.step()) {
                return true;
            }
        }
#endif

        lcdController.drawScanline();
        lcdController.onHBlank();
        cpu.dmaGroup.triggerCondition(gbaemu::DMAGroup::StartCondition::WAIT_HBLANK);

#ifndef DEBUG_CLI
        executionInfo = cpu.step(272);
        if (executionInfo != gbaemu::CPUExecutionInfoType::NORMAL) {
            std::cout << "CPU error occurred: " << std::endl;
            std::cout << cpu.state.executionInfo.message.str() << std::endl;
            return true;
        }
#else
        for (int j = 0; j < 272; ++j) {
            if (debugCLI.step()) {
                return true;
            }
        }
#endif

        lcdController.onVCount();
    }

    lcdController.onVBlank();
    cpu.dmaGroup.triggerCondition(gbaemu::DMAGroup::StartCondition::WAIT_VBLANK);

    for (int i = 0; i < 68; ++i) {
#ifndef DEBUG_CLI
        gbaemu::CPUExecutionInfoType executionInfo = cpu.step(1232);
        if (executionInfo != gbaemu::CPUExecutionInfoType::NORMAL) {
            std::cout << "CPU error occurred: " << std::endl;
            std::cout << cpu.state.executionInfo.message.str() << std::endl;
            return true;
        }
#else
        for (int j = 0; j < 1232; ++j) {
            if (debugCLI.step()) {
                return true;
            }
        }
#endif

        lcdController.onVCount();
    }

    lcdController.clearBlankFlags();
    lcdController.present();

    return false;
}
#else
static bool frame(gbaemu::CPU &cpu, gbaemu::lcd::LCDController &lcdController
#ifdef DEBUG_CLI
                  ,
                  gbaemu::debugger::DebugCLI &debugCLI
#endif
)
{
    for (int i = 0; i < 280896; ++i) {
#ifdef DEBUG_CLI
        if (debugCLI.step()) {
            return true;
        }
#else
        gbaemu::CPUExecutionInfoType executionInfo = cpu.step(1);
        if (executionInfo != gbaemu::CPUExecutionInfoType::NORMAL) {
            std::cout << "CPU error occurred: " << std::endl;
            std::cout << cpu.state.executionInfo.message.str() << std::endl;
            return true;
        }
#endif
        lcdController.renderTick();
    }
    return false;
}
#endif

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
#if RENDERER_USE_FB_CANVAS == 1
    if (argc <= 1) {
        std::cout << "please provide a path to a frame buffer!\n";
        return 0;
    }

    const constexpr int ROM_IDX = 2;
#else
    const constexpr int ROM_IDX = 1;
#endif

    if (argc <= ROM_IDX) {
        std::cout << "please provide a ROM file\n";
        return 0;
    }

    /* read gba file */
    std::ifstream file(argv[ROM_IDX], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open ROM file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

    /* intialize CPU and print game info */
    gbaemu::CPU cpu;

    std::string saveFileName(argv[ROM_IDX]);
    saveFileName += ".sav";
    if (!cpu.state.memory.loadROM(saveFileName.data(), reinterpret_cast<const uint8_t *>(buf.data()), buf.size())) {
        std::cout << "could not open/create save file" << std::endl;
        return 0;
    }

    if (argc > ROM_IDX + 1) {
        std::ifstream biosFile(argv[ROM_IDX + 1], std::ios::binary);

        if (!biosFile.is_open()) {
            std::cout << "could not open BIOS file\n";
            return 0;
        }

        std::vector<char> biosBuf(std::istreambuf_iterator<char>(biosFile), {});
        biosFile.close();

        std::cout << "INFO: Using external bios " << argv[ROM_IDX + 1] << std::endl;
        cpu.state.memory.loadExternalBios(reinterpret_cast<const uint8_t *>(biosBuf.data()), biosBuf.size());
    } else {
        std::cout << "WARNING: using buggy fallback bios! Please consider using an external bios rom!" << std::endl;
    }

    /* signal and window stuff */
    std::signal(SIGINT, handleSignal);

    SDL_Init(0);
    assert(SDL_InitSubSystem(SDL_INIT_EVENTS) == 0);

#if RENDERER_USE_FB_CANVAS == 0
    gbaemu::lcd::Window windowCanvas(1280, 720);
#else
    gbaemu::lcd::FBCanvas windowCanvas(argv[ROM_IDX - 1]);
#endif

    /* initialize SDL and LCD */
    gbaemu::lcd::LCDController lcdController(windowCanvas, &cpu);

    cpu.setLCDController(&lcdController);

    gbaemu::InstructionExecutionInfo _info;
    std::cout << "Game Title: ";
    for (uint32_t i = 0; i < 12; ++i) {
        std::cout << static_cast<char>(cpu.state.memory.read8(gbaemu::memory::EXT_ROM_OFFSET + 0x0A0 + i, _info, false));
    }
    std::cout << std::endl;
    std::cout << "Game Code: ";
    for (uint32_t i = 0; i < 4; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::memory::EXT_ROM_OFFSET + 0x0AC + i, _info, false) << " ";
    }
    std::cout << std::endl;
    std::cout << "Maker Code: ";
    for (uint32_t i = 0; i < 2; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::memory::EXT_ROM_OFFSET + 0x0B0 + i, _info, false) << " ";
    }
    std::cout << std::endl;

    cpu.refillPipelineAfterBranch<false>();

    std::cout << "Max legit ROM address: 0x" << std::hex << (gbaemu::memory::EXT_ROM_OFFSET + cpu.state.memory.getRomSize() - 1) << std::endl;

#ifdef DUMP_ROM
    {
        // Decode the whole rom & print disas, good the ensure decoder isnt broken after changes!
        gbaemu::Instruction instruction;
        instruction.isArm = true;
        gbaemu::InstructionExecutionInfo info{0};
        std::cout << "ARM Dump:" << std::endl;
        for (uint32_t i = gbaemu::memory::EXT_ROM_OFFSET; i < gbaemu::memory::EXT_ROM_OFFSET + cpu.state.memory.getRomSize(); i += 4) {
            instruction.inst = cpu.state.memory.readInst32(i, info);
            std::cout << instruction.toString() << std::endl;
        }

        std::cout << std::endl
                  << std::endl
                  << std::endl
                  << "THUMB Dump:" << std::endl;
        instruction.isArm = false;
        for (uint32_t i = gbaemu::memory::EXT_ROM_OFFSET; i < gbaemu::memory::EXT_ROM_OFFSET + cpu.state.memory.getRomSize(); i += 2) {
            instruction.inst = cpu.state.memory.readInst16(i, info);
            std::cout << instruction.toString() << std::endl;
        }
    }
#endif

    gbaemu::keyboard::KeyboardController gameController(cpu.keypad);

#ifdef DEBUG_CLI
    gbaemu::debugger::DebugCLI debugCLI(cpu, lcdController);
#endif

#ifdef DEBUG_CLI
    std::cout << "INFO: Launching CLI thread" << std::endl;
    std::thread cliThread(CLILoop, std::ref(debugCLI));
#endif

    using frames = std::chrono::duration<int64_t, std::ratio<1, 60>>; // 60Hz
#if LIMIT_FPS
    auto nextFrame = std::chrono::system_clock::now() + frames{0};
#endif

#if !defined(DEBUG_CLI) && PRINT_FPS
    auto lastFrame = std::chrono::system_clock::now() + frames{0};
#endif

    for (; doRun;) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE) {
                goto breakOuterLoop;
            }
            gameController.processSDLEvent(event);
        }

        if (frame(cpu, lcdController
#ifdef DEBUG_CLI
                  ,
                  debugCLI
#endif
                  )) {
            break;
        }

        windowCanvas.present();

#if LIMIT_FPS
        std::this_thread::sleep_until(nextFrame);
        nextFrame += frames{1};
#endif

#if !defined(DEBUG_CLI) && PRINT_FPS
        auto currentTime = std::chrono::system_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastFrame);
        std::cout << "Current FPS: " << (1000000.0 / dt.count()) << std::endl;
        lastFrame = currentTime;
#endif
    }
breakOuterLoop:

    doRun = false;

    std::cout << "window closed" << std::endl;

#ifdef DEBUG_CLI
    /* When CLI is attached only quit command will exit the program! */
    cliThread.join();
#endif

    SDL_Quit();

    return 0;
}
