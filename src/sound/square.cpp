#include "square.hpp"
#include "cpu/cpu.hpp"
#include "orchestrator.hpp"
#include "util.hpp"

#include <algorithm>

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

    SquareWaveChannel::SquareWaveChannel(CPU *cpu, SoundOrchestrator &orchestrator, SoundChannel channel, uint16_t *registers) : orchestrator(orchestrator), channel(channel), registers(registers)
    {
        cpu->state.memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                SOUND_CONTROL_REG_ADDR + sizeof(regs) * channel,
                SOUND_CONTROL_REG_ADDR + sizeof(regs) * channel + sizeof(regs) - 1 - sizeof(uint16_t) * channel,
                std::bind(&SquareWaveChannel::read8FromReg, this, std::placeholders::_1),
                std::bind(&SquareWaveChannel::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&SquareWaveChannel::read8FromReg, this, std::placeholders::_1),
                std::bind(&SquareWaveChannel::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
        chunk = nullptr;
        reset();
    }

    void SquareWaveChannel::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);

        playing = false;

        timed_active = false;
        timed_cyclesRemaining = 0;
           
        env_value = 0;
        env_active = false;
        env_cyclesRemaining = 0;

        sweep_cyclesWaited = 0;

        if (chunk != nullptr)
            delete chunk;
        
        chunk = nullptr;

        reg_soundLength = 0;
        reg_dutyCycle = 0;
        reg_envStepTime = 0;
        reg_envMode = false;
        reg_envInitVal = 0;
        reg_frequency = 0;
        reg_timed = false;
        reg_reset = false;
        reg_sweepShifts = 0;
        reg_sweepDirection = false;
        reg_sweepTime = 0;

    }

    SquareWaveChannel::~SquareWaveChannel()
    {
        if (chunk != nullptr)
            delete chunk;
    }

    float SquareWaveChannel::getBaseFrequency() const 
    {
        return 4194304.0 / static_cast<float>(32 * (2048 - reg_frequency));
    }

    uint32_t SquareWaveChannel::getCyclesForEnvelope() const
    {
        // The envelope step time is the delay between successive envelope
        // increase or decrease. It is given by the following formula:
        //   T=register value*(1/64) seconds.
        return static_cast<uint32_t>(reg_envStepTime) * (CYCLES_PER_S / 64);
    }

    uint32_t SquareWaveChannel::getCyclesForSoundLength() const
    {
        // The sound length is an 6 bit value obtained from
        // the following formula:
        //   Sound length= (64-register value)*(1/256) seconds.
        return (64 - static_cast<uint32_t>(reg_soundLength)) * (CYCLES_PER_S / 256);
    }

    uint8_t SquareWaveChannel::read8FromReg(uint32_t offset) const
    {
        if (channel == CHAN_2)
            offset += sizeof(regs.soundCntL);

        return *(offset + reinterpret_cast<const uint8_t *>(&regs));
    }

    void SquareWaveChannel::write8ToReg(uint32_t offset, uint8_t value)
    {
        if (channel == CHAN_2)
            offset += sizeof(regs.soundCntL);

        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

        //TODO this can be more granular if needed!
        update = true;

        switch (offset) {
            case offsetof(SquareWaveRegs, soundCntL) + 1:
            case offsetof(SquareWaveRegs, soundCntL): {
                const uint16_t regCntL = le(regs.soundCntL);
                // Extract the register values of reg L (Channel 2 does not support sweeps)
                reg_sweepShifts = bitGet(regCntL, SOUND_SQUARE_CHANNEL_L_SHIFTS_MASK, SOUND_SQUARE_CHANNEL_L_SHIFTS_OFF);
                reg_sweepDirection = isBitSet<uint16_t, SOUND_SQUARE_CHANNEL_L_DIR_OFF>(regCntL);
                reg_sweepTime = bitGet(regCntL, SOUND_SQUARE_CHANNEL_L_TIME_MASK, SOUND_SQUARE_CHANNEL_L_TIME_OFF);
                break;
            }
            case offsetof(SquareWaveRegs, soundCntH_L) + 1:
            case offsetof(SquareWaveRegs, soundCntH_L): {
                const uint16_t regCntH_L = le(regs.soundCntH_L);
                // Extract the register values of reg H (L for channel 2)
                reg_soundLength = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK, SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF);
                reg_dutyCycle = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK, SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF);
                reg_envStepTime = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK, SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF);
                reg_envMode = isBitSet<uint16_t, SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF>(regCntH_L);
                reg_envInitVal = bitGet(regCntH_L, SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK, SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF);
                break;
            }
            case offsetof(SquareWaveRegs, soundCntX_H) + 1:
            case offsetof(SquareWaveRegs, soundCntX_H): {
                const uint16_t regCntX_H = le(regs.soundCntX_H);
                // Extract the register values of reg X (H for channel 2)
                reg_frequency = bitGet(regCntX_H, SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK, SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF);
                reg_timed = bitGet(regCntX_H, SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK, SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF);
                reg_reset = bitGet(regCntX_H, SOUND_SQUARE_CHANNEL_X_RESET_MASK, SOUND_SQUARE_CHANNEL_X_RESET_OFF);
                break;
            }
        }
    }

    void SquareWaveChannel::onRegisterUpdated()
    {
        if (reg_reset) {
            // When reset is set to 1, the envelope is resetted to its initial value
            // and sound restarts at the specified frequency.
            env_value = reg_envInitVal;
            env_cyclesRemaining = getCyclesForEnvelope();

            // Sound length is also reset
            timed_active = reg_timed;
            timed_cyclesRemaining = getCyclesForSoundLength();

            // We applied the update.
            regs.regCntX_H = le(le(regs.regCntX_H) & ~SOUND_SQUARE_CHANNEL_X_RESET_MASK);
            // And we are playing!
            playing = true;
            orchestrator.setChannelPlaybackStatus(channel, true);
        }

        if (reg_sweepTime == 0) {
            sweep_cyclesWaited = 0;
        } 

        if (reg_envStepTime == 0) {
            env_active = false;
            env_value = 0xF;
        } else {
            // The env shall be active if we can step in any direction without hitting the bounds
            env_active = (!reg_envMode && (env_value != 0x0)) && (reg_envMode && (env_value != 0xF));
        }
    }

    void SquareWaveChannel::onCheckForAdjustment(uint32_t cycles)
    {

        LOG_SOUND(std::cout << "DEBUG: Checking channel " << channel << " after " << cycles << " cycles." << std::endl);

        if (timed_active) {
            timed_cyclesRemaining -= cycles;
            if (timed_cyclesRemaining <= 0) {
                LOG_SOUND(std::cout << "       Sound timeout reached!" << std::endl);
                playing = false;
                timed_cyclesRemaining = 0;
            }
        }

        if (reg_sweepTime != 0) {
            sweep_cyclesWaited += cycles;
            if (SWEEP_TIME_CYCLES[reg_sweepTime] <= sweep_cyclesWaited) {
                
                LOG_SOUND(std::cout << "       Sweep changing! Current shifts: " << reg_sweepShifts << std::endl);
                if (reg_sweepDirection) {
                    reg_sweepShifts -= 1;
                } else {
                    reg_sweepShifts += 1;
                }
                LOG_SOUND(std::cout << "         Adjusted shifts: " << reg_sweepShifts << std::endl);

                float period = 1.0 / getBaseFrequency();
                period += period / (reg_sweepShifts << 1);
                float frequency = 1.0 / period;
                LOG_SOUND(std::cout << "         Resulting frequency: " << frequency << std::endl);

                if (frequency < 0) {
                    reg_sweepShifts += 1;
                    LOG_SOUND(std::cout << "         Frequency lower than 0Hz. Adjusting shift: " << reg_sweepShifts << std::endl);
                }
                if (frequency > 131) {
                    playing = false;
                    LOG_SOUND(std::cout << "         Frequency higher than 131Hz. Stopping playback!" << std::endl);
                }
                
                sweep_cyclesWaited -= SWEEP_TIME_CYCLES[reg_sweepTime];
                LOG_SOUND(std::cout << "       Next update will trigger in " << SWEEP_TIME_CYCLES[reg_sweepTime] << " cycles!" << std::endl);
            }
        }

        if (env_active) {
            env_cyclesRemaining -= cycles;
            if (env_cyclesRemaining <= 0) {

                LOG_SOUND(std::cout << "       Env changing! Current value: " << env_value << std::endl);

                if (reg_envMode) {
                    env_value += 1;
                    // If we reached the maximum value we do not need to step any longer
                    if (env_value == 0xFF)
                        env_active = false;
                } else {
                    env_value -= 1;
                    // If we reached the minimum value we do not need to step any longer
                    if (env_value == 0)
                        env_active = false;
                }
                LOG_SOUND(std::cout << "         Changed value: " << env_value << " Active:" << env_active << std::endl);

                if (env_active)
                    env_cyclesRemaining += getCyclesForEnvelope();
            }
        }
    }

    void SquareWaveChannel::refresh()
    {

        LOG_SOUND(std::cout << "DEBUG: Refreshing channel " << channel "!" << std::endl);
         
        
        if (!playing) {
            LOG_SOUND(std::cout << "       Playback stopped!" << std::endl);
            orchestrator.setChannelPlaybackStatus(channel, false);
            Mix_HaltChannel(channel);

            // Remove the previous chunk
            if (chunk != nullptr)
                delete chunk;

            chunk = nullptr;
            return;
        }

        // The period of one square in s
        float periodSquare = 1.0 / getBaseFrequency();
        LOG_SOUND(std::cout << "       Base period lenght " << periodSquare << std::endl);
        if (reg_sweepTime != 0) {
            // Sweep shifts bits controls the amount of change in frequency 
            // (either increase or decrease) at each change. The wave's new 
            // period is given by: T=T±T/(2^n) where n is the sweep shifts value.
            periodSquare += (periodSquare / (reg_sweepShifts << 1));
            LOG_SOUND(std::cout << "       Period with shift: " << periodSquare << std::endl);
        }

        // How long one output period is in s
        float periodOutput = 1.0 / static_cast<float>(MIX_DEFAULT_FREQUENCY);
        // How many samples we have to generate for one square period
        uint32_t samples = static_cast<uint32_t>(periodSquare / periodOutput);
        LOG_SOUND(std::cout << "       Output period: " << periodOutput << " Samples per Square: " << samples << std::endl);

        // The percantage of samples that must be high
        float duty = DUTY_CYCLES[reg_dutyCycle];
        // How many samples should be high in the current square
        uint32_t samplesHigh = static_cast<uint32_t>(samples * duty);
        LOG_SOUND(std::cout << "       Duty Cycle: " << duty << " Samples high: " << samplesHigh << std::endl);

        uint8_t amplitude = 0xF;
        if (reg_envStepTime != 0) {
            amplitude = static_cast<uint8_t>(env_value * SOUND_SQUARE_AMPLITUDE_SCALING);
        }
        LOG_SOUND(std::cout << "       Amplitude: " << amplitude << std::endl);


        Mix_Chunk *updatedChunk = new Mix_Chunk;
        // Free the sample data buffer automatically when the chunk is freed.
        updatedChunk->allocated = 1;
        updatedChunk->alen = samples;
        updatedChunk->volume = MIX_MAX_VOLUME;

        // The buffer that will store the raw quare wave data
        uint8_t *sampleBuffer = new uint8_t[samples]();
        updatedChunk->abuf = sampleBuffer;

        // Generate the samples
        std::fill_n(sampleBuffer, samplesHigh, amplitude);
        // std::fill_n(sampleBuffer + samplesHigh, samples - samplesHigh, 0);

        // Clear previous tone
        Mix_HaltChannel(channel);
        // Start playing the updated chunk
        Mix_PlayChannel(channel, updatedChunk, -1);
        LOG_SOUND(std::cout << "       Posted update to SDL2_Mixer! " << std::endl);

        // Remove the previous chunk
        if (chunk != nullptr)
            delete chunk;
        // And replace the new one
        chunk = updatedChunk;
    }

} // namespace gbaemu
