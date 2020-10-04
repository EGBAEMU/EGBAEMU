#ifndef SQUARE_CHANNEL_HPP
#define SQUARE_CHANNEL_HPP

#include "orchestrator.hpp"
#include <chrono>

#include "packed.h"

// forward declaration
struct Mix_Chunk;

namespace gbaemu
{

    // forward declaration
    class CPU;

    class SquareWaveChannel
    {
      private:
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_L_SHIFTS_OFF = 0;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_L_DIR_OFF = 3;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_L_TIME_OFF = 4;

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_SHIFTS_MASK = static_cast<uint16_t>(0b111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_DIR_MASK = static_cast<uint16_t>(0b1);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_TIME_MASK = static_cast<uint16_t>(0b111);

        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF = 0;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF = 6;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF = 8;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF = 11;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF = 12;

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK = static_cast<uint16_t>(0x11'1111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK = static_cast<uint16_t>(0b11);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK = static_cast<uint16_t>(0b111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_MODE_MASK = static_cast<uint16_t>(0b1);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK = static_cast<uint16_t>(0b1111);

        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF = 0;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF = 14;
        static const constexpr uint8_t SOUND_SQUARE_CHANNEL_X_RESET_OFF = 15;

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK = static_cast<uint16_t>(0b111'1111'1111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK = static_cast<uint16_t>(0b1);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_RESET_MASK = static_cast<uint16_t>(0b1);

        static const constexpr float SOUND_SQUARE_AMPLITUDE_SCALING = 15.9375;

        static const constexpr uint32_t CYCLES_PER_S = 16000000;

        static const constexpr float DUTY_CYCLES[4] = {
            0.125f,
            0.25f,
            0.50f,
            0.75f};

        static const constexpr uint32_t SWEEP_TIME_CYCLES[8] = {
            // The first index encodes for disabled
            0,
            // 7.8ms @ 16Mhz = (16 000 000 / 1000) * 7.8
            (CYCLES_PER_S / 1000) * 7.8,
            (CYCLES_PER_S / 1000) * 15.6,
            (CYCLES_PER_S / 1000) * 23.4,
            (CYCLES_PER_S / 1000) * 31.3,
            (CYCLES_PER_S / 1000) * 39.1,
            (CYCLES_PER_S / 1000) * 46.9,
            (CYCLES_PER_S / 1000) * 54.7,
        };

        static const constexpr uint32_t SOUND_CONTROL_REG_ADDR = Memory::IO_REGS_OFFSET + 0x60;

      public:
        enum SoundChannel : uint8_t {
            CHAN_1 = 0,
            CHAN_2,
        };

        SquareWaveChannel(CPU *cpu, SoundOrchestrator &orchestrator, SoundChannel channel, uint16_t *registers);

        ~SquareWaveChannel();

        void onCheckForAdjustment(uint32_t cycles);

      private:
        PACKED_STRUCT(SquareWaveRegs, regs,
                      uint16_t soundCntL;
                      uint16_t soundCntH_L;
                      uint16_t soundCntX_H;);

        // The superordinate sound orchestrator
        SoundOrchestrator &orchestrator;
        // The sound channel to use
        SoundChannel channel;
        // Wheather the channel is currently playing
        bool playing;

        // The current env value
        uint8_t env_value;
        // Wheather stepping the current env is okay
        bool env_active;
        // How many cycles we still need to wait for the next env step
        int32_t env_cyclesRemaining;

        // If the timed mode is still active
        bool timed_active;
        // For how many cycles the sound should be playing.
        int32_t timed_cyclesRemaining;

        int32_t sweep_cyclesWaited;

        // The last mix chunk used as container for each sample
        Mix_Chunk *chunk;

        // Extracted reg values
        uint8_t reg_sweepShifts;
        bool reg_sweepDirection;
        uint8_t reg_sweepTime;
        uint8_t reg_soundLength;
        uint8_t reg_dutyCycle;
        uint8_t reg_envStepTime;
        bool reg_envMode;
        uint8_t reg_envInitVal;
        uint16_t reg_frequency;
        bool reg_timed;
        bool reg_reset;

     

        float getBaseFrequency() const;

        uint32_t getCyclesForEnvelope() const;

        uint32_t getCyclesForSoundLength() const;

        bool onRegisterUpdated();


        void refresh();

        uint8_t read8FromReg(uint32_t offset) const;

        void write8ToReg(uint32_t offset, uint8_t value);

        void reset();
    };

} // namespace gbaemu

#endif