#ifndef SQUARE_CHANNEL_HPP
#define SQUARE_CHANNEL_HPP

#include <chrono>
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


            void onRegisterUpdated();

            void onCheckForAdjustment();

        private:

            // The superordinate sound orchestrator
            SoundOrchestrator* orchestrator;
            // The sound channel to use
            uint8_t channel;

            // Wether some register value was updated and now the sound needs an update
            bool update;
            // Pointers to the corresponding control registers
            uint16_t* registers[2] = {
                0x0000,
                0x0000
            }; 

            // The current env value
            uint8_t env_value;
            // Wheather stepping the current env is okay
            bool    env_active;
            // The last time we update the envelope of this channel
            std::chrono::steady_clock::time_point env_lastUpdate;
            // The current threshold after wich the env steps
            std::chrono::microseconds             env_updateThreshold;

            // The last mix chunk used as container for each sample
            Mix_Chunk* chunk;

            // The extracted register values for reg 1
            uint8_t reg_soundLength;
            uint8_t reg_dutyCycle;
            uint8_t reg_envStepTime;
            bool    reg_envMode;
            uint8_t reg_envInitVal;

            // The extracted register values for reg 2
            uint16_t reg_frequency;
            bool     reg_timed;
            bool     reg_reset;

            void getRegisterValues();

    };

}

#endif SQUARE_CHANNEL_HPP