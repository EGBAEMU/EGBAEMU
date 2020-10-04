#ifndef ORCHESTRATOR_HPP
#define ORCHESTRATOR_HPP

#include "packed.h"
#include "square.hpp"

#include <cstdint>

namespace gbaemu
{
    class CPU;

    class SoundOrchestrator
    {
      public:
        enum ChannelType : uint8_t {
            SOUND_1_SQUARE,
            SOUND_2_SQUARE,
            SOUND_3_WAVE,
            SOUND_4_NOISE,
            SOUND_5_FIFO_A,
            SOUND_6_FIFO_B
        };

      private:
        CPU *cpu;

        /*
            0x04000060 	REG_SOUND1CNT_L 	Sound 1 Sweep control
            0x04000062 	REG_SOUND1CNT_H 	Sound 1 Lenght, wave duty and envelope control
            0x04000064 	REG_SOUND1CNT_X 	Sound 1 Frequency, reset and loop control
            0x04000068 	REG_SOUND2CNT_L 	Sound 2 Lenght, wave duty and envelope control
            0x0400006C 	REG_SOUND2CNT_H 	Sound 2 Frequency, reset and loop control
            0x04000070 	REG_SOUND3CNT_L 	Sound 3 Enable and wave ram bank control
            0x04000072 	REG_SOUND3CNT_H 	Sound 3 Sound lenght and output level control
            0x04000074 	REG_SOUND3CNT_X 	Sound 3 Frequency, reset and loop control
            0x04000078 	REG_SOUND4CNT_L 	Sound 4 Lenght, output level and envelope control
            0x0400007C 	REG_SOUND4CNT_H 	Sound 4 Noise parameters, reset and loop control
            0x04000080 	REG_SOUNDCNT_L 	Sound 1-4 Output level and Stereo control
            0x04000082 	REG_SOUNDCNT_H 	Direct Sound control and Sound 1-4 output ratio
            0x04000084 	REG_SOUNDCNT_X 	Master sound enable and Sound 1-4 play status
            0x04000088 	REG_SOUNDBIAS 	Sound bias and Amplitude resolution control
            0x04000090 	REG_WAVE_RAM0_L 	Sound 3 samples 0-3
            0x04000092 	REG_WAVE_RAM0_H 	Sound 3 samples 4-7
            0x04000094 	REG_WAVE_RAM1_L 	Sound 3 samples 8-11
            0x04000096 	REG_WAVE_RAM1_H 	Sound 3 samples 12-15
            0x04000098 	REG_WAVE_RAM2_L 	Sound 3 samples 16-19
            0x0400009A 	REG_WAVE_RAM2_H 	Sound 3 samples 20-23
            0x0400009C 	REG_WAVE_RAM3_L 	Sound 3 samples 23-27
            0x0400009E 	REG_WAVE_RAM3_H 	Sound 3 samples 28-31
            0x040000A0 	REG_FIFO_A_L 	Direct Sound channel A samples 0-1
            0x040000A2 	REG_FIFO_A_H 	Direct Sound channel A samples 2-3
            0x040000A4 	REG_FIFO_B_L 	Direct Sound channel B samples 0-1
            0x040000A6 	REG_FIFO_B_H 	Direct Sound channel B samples 2-3
            */

        PACK_STRUCT(SoundControlRegs, regs,
                    // Control regs for channel 1
                    uint16_t sound1CntL;
                    uint16_t sound1CntH;
                    uint16_t sound1CntX;
                    // Control regs for channel 2
                    uint16_t sound2CntL;
                    uint16_t sound2CntH;
                    // Control regs for channel 3
                    uint16_t sound3CntL;
                    uint16_t sound3CntH;
                    uint16_t sound3CntX;
                    // Control regs for channel 4
                    uint16_t sound4CntL;
                    uint16_t sound4CntH;
                    // Master control regs
                    uint16_t soundCntL;
                    uint16_t soundCntH;
                    uint16_t soundCntX;
                    uint16_t soundBias;
                    // Sample registers for channel 3
                    uint16_t waveRam0L;
                    uint16_t waveRam0H;
                    uint16_t waveRam1L;
                    uint16_t waveRam1H;
                    uint16_t waveRam2L;
                    uint16_t waveRam2H;
                    uint16_t waveRam3L;
                    uint16_t waveRam3H;
                    // FIFO regs for channel 5
                    uint16_t fifoAL;
                    uint16_t fifoAH;
                    // FIFO regs for channel 6
                    uint16_t fifoBL;
                    uint16_t fifoBH;);

        uint8_t read8FromReg(uint32_t offset) const;

        void internalWrite8ToReg(uint32_t offset, uint8_t value);

        void externalWrite8ToReg(uint32_t offset, uint8_t value);

      public:
        static const uint32_t SOUND_CONTROL_REG_ADDR;

        SoundOrchestrator(CPU *cpu);

        ~SoundOrchestrator();

        void refresh();

        // Callback for SDL2 Mixer
        // void onChannelDoneCallback(int channel);
        // Callback for Channels
        void onChannelCompleted(uint8_t channel);

        void setChannelPlaybackStatus(uint8_t channel, bool playing);

      private:
        SquareWaveChannel* channel2;

    };
} // namespace gbaemu

#endif