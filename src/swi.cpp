#include "swi.hpp"
#include "regs.hpp"

#include <cmath>

namespace gbaemu
{
    namespace swi
    {

        SWIHandler softReset;
        SWIHandler registerRamReset;
        SWIHandler halt;
        SWIHandler stop;
        SWIHandler intrWait;
        SWIHandler vBlankIntrWait;

        /*
        Output:
            r0: numerator / denominator
            r1: numerator % denominator
            r3: abs(numerator / denominator)
        */
        static void _div(uint32_t *const *const currentRegs, int32_t numerator, int32_t denominator)
        {
            //TODO how to handly div by 0?

            int32_t div = numerator / denominator;

            *currentRegs[regs::R0_OFFSET] = static_cast<uint32_t>(div);
            // manually calculate remainder, because % isn't well defined for signed numbers across different platforms
            *currentRegs[regs::R1_OFFSET] = static_cast<uint32_t>(numerator - denominator * div);
            *currentRegs[regs::R3_OFFSET] = static_cast<uint32_t>(div < 0 ? -div : div);
        }
        void div(CPUState *state)
        {
            auto currentRegs = state->getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            _div(currentRegs, numerator, denominator);
        }

        void divArm(CPUState *state)
        {
            auto currentRegs = state->getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            _div(currentRegs, numerator, denominator);
        }

        void sqrt(CPUState *state)
        {
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            r0 = static_cast<int32_t>(std::sqrt(r0));
        }

        static double convertFromQ1_14ToFP(uint16_t fixedPnt) {
            uint16_t fixedPntPart = fixedPnt & 0x7FFF;
            double convertedFP = fixedPntPart / static_cast<double>(1 << 14);
            if (fixedPnt & 0x8000) {
                convertedFP = -convertedFP;
            }
            return convertedFP;
        }

        /*
        Calculates the arc tangent.
          r0   Tan, 16bit (1bit sign, 1bit integral part, 14bit decimal part)
        Return:
          r0   "-PI/2<THETA/<PI/2" in a range of C000h-4000h.
        Note: there is a problem in accuracy with "THETA<-PI/4, PI/4<THETA".
        */
        void arcTan(CPUState *state)
        {
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            double convertedFP = convertFromQ1_14ToFP(static_cast<uint16_t>(r0 & 0x0000FFFF));
            convertedFP = std::atan(convertedFP);
            bool negative = false;
            if (convertedFP < 0) {
                negative = true;
                convertedFP = -convertedFP;
            }
            
            // Upscale for conversion to fixed point
            convertedFP *= (1 << 14);
            // recreate fixed point format 16bit (1bit sign, 1bit integral part, 14bit decimal part)
            uint16_t fixedPntPart = (negative ? 0x8000 : 0x0000) | (static_cast<uint16_t>(convertedFP) & 0x7FFF);

            //TODO is there an interval reduction: -PI/2<THETA/<PI/2" in a range of C000h-4000h 
            //TODO Q1.14 would probably be in a range of 0x0000-0x7FFF
            //TODO we probably need to fix this :/
            r0 = fixedPntPart;
        }

        //TODO what is meant by correction processing?
        /*
        Calculates the arc tangent after correction processing.
        Use this in normal situations.
          r0   X, 16bit (1bit sign, 1bit integral part, 14bit decimal part)
          r1   Y, 16bit (1bit sign, 1bit integral part, 14bit decimal part)
        Return:
          r0   0000h-FFFFh for 0<=THETA<2PI.
        */
        void arcTan2(CPUState *state)
        {
        }

        SWIHandler cpuSet;
        SWIHandler cpuFastSet;
        SWIHandler biosChecksum;
        SWIHandler bgAffineSet;
        SWIHandler objAffineSet;
        SWIHandler bitUnPack;
        SWIHandler LZ77UnCompWRAM;
        SWIHandler LZ77UnCompVRAM;
        SWIHandler huffUnComp;
        SWIHandler RLUnCompWRAM;
        SWIHandler RLUnCompVRAM;
        SWIHandler diff8BitUnFilterWRAM;
        SWIHandler diff8BitUnFilterVRAM;
        SWIHandler diff16BitUnFilter;
        SWIHandler soundBiasChange;
        SWIHandler soundDriverInit;
        SWIHandler soundDriverMode;
        SWIHandler soundDriverMain;
        SWIHandler soundDriverVSync;
        SWIHandler soundChannelClear;
        SWIHandler MIDIKey2Freq;
        SWIHandler musicPlayerOpen;
        SWIHandler musicPlayerStart;
        SWIHandler musicPlayerStop;
        SWIHandler musicPlayerContinue;
        SWIHandler musicPlayerFadeOut;
        SWIHandler multiBoot;
        SWIHandler hardReset;
        SWIHandler customHalt;
        SWIHandler soundDriverVSyncOff;
        SWIHandler soundDriverVSyncOn;
        SWIHandler getJumpList;

    } // namespace swi
} // namespace gbaemu