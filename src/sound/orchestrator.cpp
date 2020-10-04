#include "orchestrator.hpp"
#include "cpu/cpu.hpp"
#include "cpu/regs.hpp"
#include "io/memory.hpp"
#include "logging.hpp"
#include "util.hpp"

#include <iostream>

#ifdef __clang__
/*code specific to clang compiler*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#elif __GNUC__
/*code for GNU C compiler */
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#elif _MSC_VER
/*usually has the version number in _MSC_VER*/
/*code specific to MSVC compiler*/
#include <SDL.h>
#include <SDL_mixer.h>
#elif __MINGW32__
/*code specific to mingw compilers*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#else
#error "unsupported compiler!"
#endif

namespace gbaemu
{
    //TODO callback is registered with function pointer... not sure if this excludes usage with Methods as they have an implicit additional argument (this pointer)
    static void onChannelDoneCallback(int channel)
    {
        std::cout << "Channel " << channel << " completed playback!" << std::endl;
    }

    uint8_t SoundOrchestrator::read8FromReg(uint32_t offset) const
    {
        return *(offset + reinterpret_cast<const uint8_t *>(&regs));
    }

    void SoundOrchestrator::internalWrite8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
    }

    void SoundOrchestrator::externalWrite8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
    }

    SoundOrchestrator::SoundOrchestrator(CPU *cpu) : channel1(cpu, *this, SquareWaveChannel::CHAN_1), channel2(cpu, *this, SquareWaveChannel::CHAN_2)
    {
        // Initialize SDL audio modules
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            printf("SDL_Init: %s\n", SDL_GetError());
            return;
        }

        // Initialize SDL Mixer. We use the default frequency of 22050Hz where
        // each sample may be represented by a signed 8bit value. 2 channels (stereo)
        // is used with a 4096 bytes per output sample.
        if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S8, 2, 4096) == -1) {
            printf("Mix_OpenAudio: %s\n", Mix_GetError());
            return;
        }

        // The GBA has 6 sound channels, thus we need 6 individual mixing channels.
        Mix_AllocateChannels(6);
        // Set the callback for playback completion
        Mix_ChannelFinished(onChannelDoneCallback);

        cpu->state.memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                SOUND_CONTROL_REG_ADDR,
                SOUND_CONTROL_REG_ADDR + sizeof(regs) - 1,
                std::bind(&SoundOrchestrator::read8FromReg, this, std::placeholders::_1),
                std::bind(&SoundOrchestrator::externalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&SoundOrchestrator::read8FromReg, this, std::placeholders::_1),
                std::bind(&SoundOrchestrator::internalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2)));

        reset();
    }

    void SoundOrchestrator::reset() {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
        channel1.reset();
        channel2.reset();
    }

    SoundOrchestrator::~SoundOrchestrator()
    {
        // Shutdown the mixer API
        Mix_CloseAudio();
        // Indicate desire to unload dll's
        Mix_Quit();
    }

    void SoundOrchestrator::setChannelPlaybackStatus(uint8_t channel, bool playing)
    {   
        // Bits 0-3 are set when their respective sound channels are playing 
        // and are resetted when sound has stopped. Note that contrary to some 
        // other sources and most emulators, these bits are read-only and do 
        // not need to be set to enable the sound channels. 
        regs.soundCntX = (le(regs.soundCntX) & (static_cast<uint16_t>(0x1) << channel)) | (static_cast<uint16_t>(playing) << channel);
    }

} // namespace gbaemu