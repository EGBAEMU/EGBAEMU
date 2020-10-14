#include "virtualKeyboard.hpp"

#include <iostream>

#include <wiringPi.h>
#include <wiringSerial.h>

#ifdef __clang__
/*code specific to clang compiler*/
#include <SDL2/SDL.h>
#elif __GNUC__
/*code for GNU C compiler */
#include <SDL2/SDL.h>
#elif _MSC_VER
/*usually has the version number in _MSC_VER*/
/*code specific to MSVC compiler*/
#include <SDL.h>
#elif __MINGW32__
/*code specific to mingw compilers*/
#include <SDL2/SDL.h>
#else
#error "unsupported compiler!"
#endif

//#define SERIAL_PORT "/dev/serial0"
#define SERIAL_PORT "/dev/ttyAMA0"
//#define SERIAL_PORT "/dev/ttyS0"

#define BAUD_RATE 115200

static const SDL_Keycode keyMapping[] = {
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_UP,
    SDLK_DOWN,
    // a button
    SDLK_j,
    // b button
    SDLK_k,
    // start button
    SDLK_RETURN,
    // select button
    SDLK_ESCAPE,
    // L button
    SDLK_l,
    // R button
    SDLK_p,
};

void virtualKeyboardLoop(volatile bool &run)
{
    int serialPort = serialOpen(SERIAL_PORT, BAUD_RATE);
    if (serialPort < 0) {
        std::cout << "Failed to open serial port: " SERIAL_PORT << std::endl;
        run = false;
        return;
    }
    if (wiringPiSetup() == -1) {
        std::cout << "WiringPi Setup failed!" << std::endl;
        run = false;
        return;
    }

    SDL_Event event;

    for (; run && serialDataAvail(serialPort) >= 0;) {
        char c = serialGetchar(serialPort);
        // the messages are interpreted as follow: up to 7 bits for the index and the LSB bit for the key state
        // Key press is encoded as 1 & key release is encoded as 0
        uint8_t index = static_cast<uint8_t>(c) >> 1;
        if (index < sizeof(keyMapping) / sizeof(keyMapping[0])) {
            event.type = (c & 1) ? SDL_KEYDOWN : SDL_KEYUP ;
            event.key.keysym.sym = keyMapping[index];
            SDL_PushEvent(&event);
        }
    }

    run = false;

    serialClose(serialPort);
}
