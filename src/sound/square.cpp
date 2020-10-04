#include "square.hpp"
#include "orchestrator.hpp"
#include "util.hpp"

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

    SquareWaveChannel::SquareWaveChannel(SoundOrchestrator *orchestrator, uint8_t channel, uint16_t *registers[2]) : orchestrator(orchestrator), channel(channel), registers(registers)
    {
    }

    SquareWaveChannel::~SquareWaveChannel()
    {
        if (chunk != nullptr)
            delete chunk;
    }

    void SquareWaveChannel::onRegisterUpdated()
    {
        // All parameters can be changed dynamically while the sound is playing!
        // Thus remember that some update has occured and adjust in next cycle.
        update = true;
    }

    void SquareWaveChannel::onCheckForAdjustment()
    {

        bool trigger = update;

        // Only check env if its active!
        if (env_active) {

            // The current time
            auto timeCurrent = std::chrono::high_resolution_clock::now();
            // The elapsed time since the start if the current env
            //TODO @schmax does not compile...
            uint32_t timeElapsed /*= std::chrono::duration_cast<std::chrono::microseconds>(timeCurrent - env_lastUpdate).count()*/;

            // The current env step has expired! Advance to the next one!
            if (std::chrono::microseconds(timeElapsed) >= env_updateThreshold) {

                // Make sure we update the sound later on
                trigger = true;

                // Upate the env value
                if (reg_envMode) {
                    // Increase env value to the next value
                    env_value += 1;
                    // If we reaced the maximum value we do not need to step any longer
                    if (env_value == 0xFF)
                        env_active = false;

                } else {
                    // Decrease env by one
                    env_value -= 1;
                    // If we reached the minimum value we do not need to step any longer
                    if (env_value == 0)
                        env_active = false;
                }
            }
        }

        // Fetch register values if some value has been refreshed
        if (update)
            getRegisterValues();

        if (trigger)
            refresh();

        update = false;
    }

    void SquareWaveChannel::refresh()
    {

        if (reg_reset) {
            // When reset is set to 1, the envelope is resetted to its initial value 
            // and sound restarts at the specified frequency. 
            env_value = reg_envInitVal;

            // TODO: Set reset register to 0

        }

        // If the step time is 0 the env is disabled
        if (reg_envStepTime == 0) {
            // Disable check for stepping
            env_active = false;
            // Tie the amplitude to max (Envelope value has 4 bits)
            env_value = 0xF;
        } else {
            // Update the env step delta. A update may occur after (reg_value * 1 / 64) seconds.
            env_updateThreshold = std::chrono::microseconds{static_cast<uint32_t>(reg_envStepTime) * 15625};
            // The env shall be active if we can step in any direction without hitting the bounds
            env_active = (!reg_envMode && (env_value != 0x0)) && (reg_envMode && (env_value != 0xF));
        }


        // The period of one square in s
        float periodSquare = static_cast<float>(32 * (2048 - reg_frequency)) / 4194304.0;
        // How long one output period is in s
        float periodOutput = 1.0 / static_cast<float>(MIX_DEFAULT_FREQUENCY);
        // How many samples we have to generate for one square period
        uint32_t samples = static_cast<uint32_t>(periodSquare / periodOutput);

        // The percantage of samples that must be high
        float duty = DUTY_CYCLES[reg_dutyCycle];
        // How many samples should be high in the current square
        uint32_t samplesHigh = static_cast<uint32_t>(samples * duty);

        // The amplitude to use is influenced by the current env
        uint8_t amplitude = static_cast<uint8_t>(env_value * SOUND_SQUARE_AMPLITUDE_SCALING);

        Mix_Chunk *updatedChunk = new Mix_Chunk;
        // Free the sample data buffer automatically when the chunk is freed.
        updatedChunk->allocated = 1;
        updatedChunk->alen = samples;
        updatedChunk->volume = MIX_MAX_VOLUME;

        // The buffer that will store the raw quare wave data
        uint8_t *sampleBuffer = new uint8_t[samples];
        updatedChunk->abuf = sampleBuffer;

        // Generate the samples
        for (uint32_t sampleIdx = 0; sampleIdx < samples; ++sampleIdx) {
            if (sampleIdx < samplesHigh)
                sampleBuffer[sampleIdx] = amplitude;
            else
                sampleBuffer[sampleIdx] = 0x0;
        }

        // Clear previous tone
        Mix_HaltChannel(channel);
        // Start playing the updated chunk
        Mix_PlayChannel(channel, chunk, -1);

        // Remove the previous chunk
        if (chunk != nullptr)
            delete chunk;
        // And replace the new one
        chunk = updatedChunk;
    }

    void SquareWaveChannel::getRegisterValues()
    {
        const uint16_t reg1 = le(*registers[0]);
        const uint16_t reg2 = le(*registers[1]);

        // Extract the register values of reg 1
        reg_soundLength = static_cast<uint8_t>((reg1 & SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK) >> SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF);
        reg_dutyCycle = static_cast<uint8_t>((reg1 & SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK) >> SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF);
        reg_envStepTime = static_cast<uint8_t>((reg1 & SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK) >> SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF);
        reg_envMode = static_cast<bool>((reg1 & SOUND_SQUARE_CHANNEL_H_ENV_MODE_MASK) >> SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF);
        reg_envInitVal = static_cast<uint8_t>((reg1 & SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK) >> SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF);
        // Extract the register values of reg 2
        reg_frequency = static_cast<uint16_t>((reg2 & SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK) >> SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF);
        reg_timed = static_cast<bool>((reg2 & SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK) >> SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF);
        reg_reset = static_cast<bool>((reg2 & SOUND_SQUARE_CHANNEL_X_RESET_MASK) >> SOUND_SQUARE_CHANNEL_X_RESET_OFF);

    }

} // namespace gbaemu
