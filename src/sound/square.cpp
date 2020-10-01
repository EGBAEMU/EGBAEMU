#include "square.hpp"
#include "orchestrator.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

namespace gbaemu
{

    SquareWaveChannel::SquareWaveChannel(SoundOrchestrator* orchestrator, int channelID, bool sweeps, uint16_t *registers[]) : orchestrator(orchestrator), channelID(channelID), sweeps(sweeps), registers(registers)
    {
    }

    SquareWaveChannel::~SquareWaveChannel()
    {
        if (chunk != nullptr)
            delete chunk;
    }

    void SquareWaveChannel::refresh()
    {
        
        // Register values of this SquareWaveChannel
        uint16_t regCntH = *registers[1];
        uint16_t regCntX = *registers[2];

        // Decode H register values
        uint8_t soundLenght = static_cast<uint8_t>((regCntH & SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK) >> SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF);
        uint8_t dutyCycle = static_cast<uint8_t>((regCntH & SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK) >> SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF);
        uint8_t envStepTime = static_cast<uint8_t>((regCntH & SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK) >> SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF);
        uint8_t envMode = static_cast<uint8_t>((regCntH & SOUND_SQUARE_CHANNEL_H_ENV_MODE_MASK) >> SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF);
        uint8_t envInitVal = static_cast<uint8_t>((regCntH & SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK) >> SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF);

        uint16_t frequencyEncoded = static_cast<uint16_t>((regCntX & SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK) >> SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF);
        uint8_t timedMode = static_cast<uint8_t>((regCntX & SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK) >> SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF);
        uint8_t soundReset = static_cast<uint8_t>((regCntX & SOUND_SQUARE_CHANNEL_X_RESET_MASK) >> SOUND_SQUARE_CHANNEL_X_RESET_OFF);

        if (sweep) {
            // Register L may only be dereferenced if sweeps are enabled
            uint16_t regCntL = *registers[0];
        }

        // The period of one square in ms
        double period = static_cast<double>(32 * (2048 - frequencyEncoded)) / 4194304.0;
        // How long one output period is in ms
        double outputPeriod = 1.0 / static_cast<double>(MIX_DEFAULT_FREQUENCY);
        // How many samples we have to generate for one square period
        uint32_t samplesPerPeriod = static_cast<uint32_t>(period / outputPeriod);

        // The percantage of samples that must be high
        double duty = DUTY_CYCLES[dutyCycle];
        // How many samples should be high in the current square
        uint32_t samplesHigh = static_cast<uint32_t>(samplesPerPeriod * duty);


        Mix_Chunk* chunk = (Mix_Chunk*) malloc(sizeof(Mix_Chunk));
        // Free the sample data buffer automatically when the chunk is freed.
        chunk->allocated = 1;
        chunk->alen = samplesPerPeriod;
        chunk->volume = MIX_MAX_VOLUME; 

        // The buffer that will store the raw quare wave data
        Uint8* sampleBuffer = (Uint8*) malloc(samplesPerPeriod * sizeof(Uint8));
        chunk->abuf = sampleBuffer;

        // Generate the samples
        for (uint32_t sampleIdx = 0; sampleIdx < samplesPerPeriod; sampleIdx++) {
            if (sampleIdx < samplesHigh)
                sampleBuffer[sampleIdx] = 0xFF;
            else
                sampleBuffer[sampleIdx] = 0x0;
        }
        
        // Clear previous tone
        Mix_HaltChannel(0);
        // Start playing the updated chunk
        Mix_PlayChannel(0, chunk, -1);

    }
}
