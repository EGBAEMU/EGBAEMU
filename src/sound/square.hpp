#ifndef SQUARE_CHANNEL_HPP
#define SQUARE_CHANNEL_HPP

#include "io/memory.hpp"
#include "packed.h"

// forward declaration
struct Mix_Chunk;

namespace gbaemu
{
    class CPU;
} // namespace gbaemu

namespace gbaemu::sound
{

    // forward declaration
    class SoundOrchestrator;

    class SquareWaveChannel
    {
      private:
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_SHIFTS_OFF = 0;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_DIR_OFF = 3;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_TIME_OFF = 4;

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_SHIFTS_MASK = static_cast<uint16_t>(0b111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_DIR_MASK = static_cast<uint16_t>(0b1);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_L_TIME_MASK = static_cast<uint16_t>(0b111);

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_OFF = 0;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_OFF = 6;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_OFF = 8;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_MODE_OFF = 11;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_OFF = 12;

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_SOUND_LENGTH_MASK = static_cast<uint16_t>(0b11'1111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_DUTY_CYCLE_MASK = static_cast<uint16_t>(0b11);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_STEP_TIME_MASK = static_cast<uint16_t>(0b111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_MODE_MASK = static_cast<uint16_t>(0b1);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_H_ENV_INIT_VAL_MASK = static_cast<uint16_t>(0b1111);

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_OFF = 0;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_TIME_MODE_OFF = 14;
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_RESET_OFF = 15;

        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_SOUND_FREQ_MASK = static_cast<uint16_t>(0b111'1111'1111);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_TIME_MODE_MASK = static_cast<uint16_t>(0b1);
        static const constexpr uint16_t SOUND_SQUARE_CHANNEL_X_RESET_MASK = static_cast<uint16_t>(0b1);

        static const constexpr uint32_t SOUND_CONTROL_REG_ADDR = Memory::IO_REGS_OFFSET + 0x60;

        PACK_STRUCT(SquareWaveRegs, regs,
                    uint16_t soundCntL;
                    uint16_t soundCntH_L;
                    uint16_t soundCntX_H;
                    uint16_t _unused;);

        /*
          A square channel's frequency timer period is set to (2048-frequency)*4. 
          Four duty cycles are available, each waveform taking 8 frequency timer 
          clocks to cycle through: 
          
          Duty   Waveform    Ratio
          -------------------------
          0      00000001    12.5%
          1      10000001    25%
          2      10000111    50%
          3      01111110    75%
        */
        static const constexpr uint8_t DUTY_CYCLE_LOOKUP[4] = {
            0b00000001,
            0b10000001,
            0b10000111,
            0b01111110,
        };

      public:
        enum SoundChannel : uint8_t {
            CHAN_1 = 0,
            CHAN_2,
        };

      private:
        // The superordinate sound orchestrator
        SoundOrchestrator *orchestrator;
        // The sound channel to use
        SoundChannel channel;

        // The current outputting volume
        uint32_t volumeOut;

        // If this channel is currently outputting anything
        bool active;
        // Counts down the progression of the sequence index
        uint32_t timer;
        // The current index in the duty cycle table
        uint8_t sequenceIdx;

        // Wheather stepping the current env is okay
        bool env_active;
        // The current internal volume
        uint32_t env_value;
        // How many cycles we still need to wait for the next env step
        uint32_t env_counter;

        // If the timed mode is still active
        bool timed_active;
        // For how many cycles the sound should be playing.
        uint32_t timed_counter;

        // If the sweep is active
        bool sweep_active;
        // The current sweep adjusted frequency
        uint32_t sweep_current;
        // Counter for the next sweep adjustment
        uint32_t sweep_counter;
        // The current frequency adjustment
        uint16_t sweep_shadow;

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

        uint8_t read8FromReg(uint32_t offset) const;

        void write8ToReg(uint32_t offset, uint8_t value);

        void onRegisterUpdated();

        void onCalculateFrequency(bool writeback);

      public:
        SquareWaveChannel(CPU *cpu, SoundOrchestrator *orchestrator, SoundChannel channel);

        void reset();

        uint16_t getCurrentVolume();

        void onStepVolume();

        void onStepEnv();

        void onStepSoundLength();

        void onStepSweep();
    };

} // namespace gbaemu::sound

#endif