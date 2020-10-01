#ifndef SQUARE_CHANNEL_HPP
#define SQUARE_CHANNEL_HPP

#include "orchestrator.hpp"

namespace gbaemu 
{



    class SquareWaveChannel
    {
        private:

            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_L_SHIFTS_OFF = 0;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_L_DIR_OFF = 3;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_L_TIME_OFF = 4;

            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_SHIFTS_MASK = static_cast<uint16_t>(3) << SOUND_SQUARE_CHANNEL_L_SHIFTS_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_DIR_MASK = static_cast<uint16_t>(1) << SOUND_SQUARE_CHANNEL_L_DIR_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_TIME_MASK = static_cast<uint16_t>(3) << SOUND_SQUARE_CHANNEL_L_TIME_OFF;

            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF = 0;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF = 6;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF = 8;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF = 11;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF = 12;

            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK = static_cast<uint16_t>(6) << SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK = static_cast<uint16_t>(2) << SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK = static_cast<uint16_t>(3) << SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_MODE_MASK = static_cast<uint16_t>(1) << SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK = static_cast<uint16_t>(4) << SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF;
            
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF = 0;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF = 14;
            static const constexpr uint8_t SOUND_SQUARE_CHANNEL_X_RESET_OFF = 15;

            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK = static_cast<uint16_t>(11) << SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK = static_cast<uint16_t>(1) << SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF;
            static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_RESET_MASK = static_cast<uint16_t>(1) << SOUND_SQUARE_CHANNEL_X_RESET_OFF;

            const float DUTY_CYCLES[4] = {
                0.125,
                0.25,
                0.50,
                0.75
            };

        public:

            SquareWaveChannel(SoundOrchestrator* orchestrator, int channelID, bool sweeps, uint16_t *registers[]);

            ~SquareWaveChannel();

            void refresh();

        private:

            SoundOrchestrator* orchestrator;
            
            // The sound channel to use
            int channelID;

            // Pointers to the corresponding control registers
            uint16_t *registers[3]; 

            // If this channel supports sweeps
            bool sweeps = false;
            // The last mix chunk used as container for each sample
            Mix_Chunk* chunk;

    };

}

#endif SQUARE_CHANNEL_HPP