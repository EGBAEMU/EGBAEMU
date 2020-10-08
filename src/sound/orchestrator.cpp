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

namespace gbaemu::sound
{

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

    SoundOrchestrator::SoundOrchestrator(CPU *cpu) : channel1(cpu, this, SquareWaveChannel::CHAN_1), channel2(cpu, this, SquareWaveChannel::CHAN_2)
    {
        // Initialize SDL audio modules
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            printf("SDL_Init: %s\n", SDL_GetError());
            return;
        }

        SDL_AudioSpec audioSpec;
        audioSpec.freq = AUDIO_FREQUENCY;
        audioSpec.format = AUDIO_F32SYS;
        audioSpec.channels = 2;
        audioSpec.samples = SOUND_OUTPUT_SAMPLE_SIZE;
        audioSpec.callback = NULL;
        audioSpec.userdata = this;

        SDL_AudioSpec obtainedSpec;
        SDL_OpenAudio(&audioSpec, &obtainedSpec);
        SDL_PauseAudio(0);

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

    void SoundOrchestrator::reset() 
    {
        
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
        
        sampling_counter = CLOCK_CYCLES_SAMPLE;
        sampling_bufferIdx = 0;

        frame_sequenceCounter = CLOCK_APU_CYCLES_SKIPS;
        frame_sequencer = 0;

        channel1.reset();
        channel2.reset();
    }

    void SoundOrchestrator::step(uint32_t cycles) 
    {
        while(cycles > 0) {
            cycles -= 1;

            onHandleFrameSequencer();
            onStepChannels();
            onHandleDownsampling();
        }
    }

    void SoundOrchestrator::onHandleFrameSequencer()
    {
        frame_sequenceCounter -= 1;
       
        if (frame_sequenceCounter != 0) 
            return;
       
        frame_sequenceCounter = CLOCK_APU_CYCLES_SKIPS;
        frame_stepLut[frame_sequencer]();
        
        frame_sequencer = (frame_sequencer + 1) & 0b111;
       
    }

    void SoundOrchestrator::onStepChannels()
    {
        channel1.onStepVolume();
        channel2.onStepVolume();
    }

    void SoundOrchestrator::onHandleDownsampling()
    {
        sampling_counter -= 1;
    
        if (sampling_counter != 0)
            return;

        sampling_counter = CLOCK_CYCLES_SAMPLE;

        float sample = 0;
        
        // TODO: We can only mix those channels who are actually active.

        float channel1Sample = ((float) channel1.getCurrentVolume()) / 100;
        SDL_MixAudioFormat((Uint8*)&sample, (Uint8*)&channel1Sample, AUDIO_F32SYS, sizeof(float), SDL_MIX_MAXVOLUME);

        float channel2Sample = ((float) channel2.getCurrentVolume()) / 50;
        SDL_MixAudioFormat((Uint8*)&sample, (Uint8*)&channel2Sample, AUDIO_F32SYS, sizeof(float), SDL_MIX_MAXVOLUME);
        
        sampling_buffer[sampling_bufferIdx] = sample;
        sampling_buffer[sampling_bufferIdx + 1] = sample;
        sampling_bufferIdx += 2;

        if (sampling_bufferIdx < SOUND_OUTPUT_SAMPLE_SIZE)
            return;

        sampling_bufferIdx = 0;

        //std::cout << "Enqueue audio for playback!" << std::endl;
        while ((SDL_GetQueuedAudioSize(1)) > SOUND_OUTPUT_SAMPLE_SIZE * sizeof(float)) {
				SDL_Delay(1);
			}
        if (SDL_QueueAudio(1, sampling_buffer, SOUND_OUTPUT_SAMPLE_SIZE * sizeof(float)) < 0) {
            printf("SDL_QueueAudio: %s\n", SDL_GetError());
        }
    }
/*
    void SoundOrchestrator::setChannelPlaybackStatus(uint8_t channel, bool playing)
    {   
        // Bits 0-3 are set when their respective sound channels are playing 
        // and are resetted when sound has stopped. Note that contrary to some 
        // other sources and most emulators, these bits are read-only and do 
        // not need to be set to enable the sound channels. 
        regs.soundCntX = le((le(regs.soundCntX) & (static_cast<uint16_t>(0x1) << channel)) | (static_cast<uint16_t>(playing) << channel));
    }
*/
} // namespace gbaemu