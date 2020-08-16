#include "swi.hpp"
#include "regs.hpp"

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>

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

        static double convertFromQ1_14ToFP(uint16_t fixedPnt)
        {
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
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            double x = convertFromQ1_14ToFP(static_cast<uint16_t>(r0 & 0x0000FFFF));
            double y = convertFromQ1_14ToFP(static_cast<uint16_t>(state->accessReg(regs::R1_OFFSET) & 0x0000FFFF));

            //TODO atan2 returns [-pi/2,pi/2] but we need [0, 2pi]... can we just make this interval transformation or is it calculated differently???
            double res = std::atan2(x, y) * 2 + M_PI;
            // Transform to integer interval [0, 0xFFFF]
            r0 = static_cast<uint32_t>(static_cast<uint16_t>((res * 0x0FFFF) / (2 * M_PI)));
        }

        //TODO Note: On GBA, NDS7 and DSi7, these two functions(cpuset & cpufastset) will silently reject to do anything if the source start or end addresses are reaching into the BIOS area. The NDS9 and DSi9 don't have such read-proctections.
        /*
        Memory copy/fill in units of 32 bytes. Memcopy is implemented as repeated LDMIA/STMIA [Rb]!,r2-r9 instructions. Memfill as single LDR followed by repeated STMIA [Rb]!,r2-r9.
        After processing all 32-byte-blocks, the NDS/DSi additonally processes the remaining words as 4-byte blocks. BUG: The NDS/DSi uses the fast 32-byte-block processing only for the first N bytes (not for the first N words), so only the first quarter of the memory block is FAST, the remaining three quarters are SLOWLY copied word-by-word.
        The length is specifed as wordcount, ie. the number of bytes divided by 4.
        On the GBA, the length should be a multiple of 8 words (32 bytes) (otherwise the GBA is forcefully rounding-up the length). On NDS/DSi, the length may be any number of words (4 bytes).
          r0    Source address        (must be aligned by 4)
          r1    Destination address   (must be aligned by 4)
          r2    Length/Mode
                  Bit 0-20  Wordcount (GBA: rounded-up to multiple of 8 words)
                  Bit 24    Fixed Source Address (0=Copy, 1=Fill by WORD[r0])
        Return: No return value, Data written to destination address.
        */
        void cpuFastSet(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint8_t *sourceAddr = *currentRegs[regs::R0_OFFSET] + static_cast<uint8_t *>(state->memory.mem);
            uint8_t *destAddr = *currentRegs[regs::R1_OFFSET] + static_cast<uint8_t *>(state->memory.mem);
            uint32_t length_mode = *currentRegs[regs::R2_OFFSET];

            uint32_t length = length_mode & 0x001FFFFF;
            uint32_t rest = length % 8;
            // ceiling to multiple of 8
            if (rest) {
                length += 8 - rest;
            }
            // convert from word(32 bit) count to byte count
            length <<= 2;

            bool fixedMode = length_mode & (1 << 24);
            if (fixedMode) {
                // Fill with value pointed to by r0
                uint32_t value = *reinterpret_cast<uint32_t *>(sourceAddr);
                uint32_t *destPtr = reinterpret_cast<uint32_t *>(destAddr);
                uint32_t *endPtr = destPtr + length / sizeof(*destPtr);
                std::fill(destPtr, endPtr, value);
            } else {
                // Normal memcpy behaviour
                std::memcpy(destAddr, sourceAddr, length);
            }
        }
        /*
        Memory copy/fill in units of 4 bytes or 2 bytes. Memcopy is implemented as repeated LDMIA/STMIA [Rb]!,r3 or LDRH/STRH r3,[r0,r5] instructions. Memfill as single LDMIA or LDRH followed by repeated STMIA [Rb]!,r3 or STRH r3,[r0,r5].
        The length must be a multiple of 4 bytes (32bit mode) or 2 bytes (16bit mode). The (half)wordcount in r2 must be length/4 (32bit mode) or length/2 (16bit mode), ie. length in word/halfword units rather than byte units.
          r0    Source address        (must be aligned by 4 for 32bit, by 2 for 16bit)
          r1    Destination address   (must be aligned by 4 for 32bit, by 2 for 16bit)
          r2    Length/Mode
                  Bit 0-20  Wordcount (for 32bit), or Halfwordcount (for 16bit)
                  Bit 24    Fixed Source Address (0=Copy, 1=Fill by {HALF}WORD[r0])
                  Bit 26    Datasize (0=16bit, 1=32bit)
        Return: No return value, Data written to destination address.
        */
        void cpuSet(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint8_t *sourceAddr = *currentRegs[regs::R0_OFFSET] + static_cast<uint8_t *>(state->memory.mem);
            uint8_t *destAddr = *currentRegs[regs::R1_OFFSET] + static_cast<uint8_t *>(state->memory.mem);
            uint32_t length_mode = *currentRegs[regs::R2_OFFSET];

            uint32_t length = length_mode & 0x001FFFFF;
            bool fixedMode = length_mode & (1 << 24);
            bool dataSize32bit = length_mode & (1 << 26);

            length <<= dataSize32bit ? 2 : 1;

            if (fixedMode) {
                // Fill with value pointed to by r0
                if (dataSize32bit) {
                    uint32_t value = *reinterpret_cast<uint32_t *>(sourceAddr);
                    uint32_t *destPtr = reinterpret_cast<uint32_t *>(destAddr);
                    uint32_t *endPtr = destPtr + length / sizeof(*destPtr);
                    std::fill(destPtr, endPtr, value);
                } else {
                    uint16_t value = *reinterpret_cast<uint16_t *>(sourceAddr);
                    uint16_t *destPtr = reinterpret_cast<uint16_t *>(destAddr);
                    uint16_t *endPtr = destPtr + length / sizeof(*destPtr);
                    std::fill(destPtr, endPtr, value);
                }
            } else {
                // Normal memcpy behaviour
                std::memcpy(destAddr, sourceAddr, length);
            }
        }

        /*
        Calculates the checksum of the BIOS ROM (by reading in 32bit units, and adding up these values). IRQ and FIQ are disabled during execution.
        The checksum is BAAE187Fh (GBA and GBA SP), or BAAE1880h (NDS/3DS in GBA mode, whereas the only difference is that the byte at [3F0Ch] is changed from 00h to 01h, otherwise the BIOS is 1:1 same as GBA BIOS, it does even include multiboot code).
        Parameters: None. Return: r0=Checksum.
        */
        void biosChecksum(CPUState *state)
        {
            state->accessReg(regs::R0_OFFSET) = 0x0BAAE18F;
        }

        SWIHandler bgAffineSet;
        SWIHandler objAffineSet;

        /*
        Used to increase the color depth of bitmaps or tile data. For example, to convert a 1bit monochrome font into 4bit or 8bit GBA tiles. The Unpack Info is specified separately, allowing to convert the same source data into different formats.
          r0  Source Address      (no alignment required)
          r1  Destination Address (must be 32bit-word aligned)
          r2  Pointer to UnPack information:
               16bit  Length of Source Data in bytes     (0-FFFFh)
               8bit   Width of Source Units in bits      (only 1,2,4,8 supported)
               8bit   Width of Destination Units in bits (only 1,2,4,8,16,32 supported)
               32bit  Data Offset (Bit 0-30), and Zero Data Flag (Bit 31)
              The Data Offset is always added to all non-zero source units.
              If the Zero Data Flag was set, it is also added to zero units.
        Data is written in 32bit units, Destination can be Wram or Vram. The size of unpacked data must be a multiple of 4 bytes. The width of source units (plus the offset) should not exceed the destination width.
        Return: No return value, Data written to destination address
        */
        void bitUnPack(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint8_t *sourceAddr = *currentRegs[regs::R0_OFFSET] + static_cast<uint8_t *>(state->memory.mem);
            uint8_t *destAddr = *currentRegs[regs::R1_OFFSET] + static_cast<uint8_t *>(state->memory.mem);
            uint32_t *unpackFormatPtr = reinterpret_cast<uint32_t *>(*currentRegs[regs::R2_OFFSET] + static_cast<uint8_t *>(state->memory.mem));

            uint32_t length_unitWidths = *unpackFormatPtr;

            //TODO i hate endians... how are the bytes stored?
            uint16_t dataByteCount = static_cast<uint16_t>((length_unitWidths >> 16) & 0x0000FFFF);
            uint8_t srcUnitWidth = static_cast<uint8_t>((length_unitWidths >> 8) & 0x000000FF);
            uint8_t destUnitWidth = static_cast<uint8_t>((length_unitWidths & 0x000000FF));
            uint32_t dataOffset = *(unpackFormatPtr + 1);
            bool zeroDataOff = dataOffset & (1 << 31);
            dataOffset &= 0x7FFFFFF;

//TODO implement unpacking... not sure how this is intended
        }

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