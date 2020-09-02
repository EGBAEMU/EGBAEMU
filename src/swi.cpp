#define _USE_MATH_DEFINES
#include <cmath>

#include "math3d.hpp"
#include "regs.hpp"
#include "swi.hpp"
#include <algorithm>
#include <cstring>

namespace gbaemu
{
    namespace swi
    {
        const char *swiToString(uint8_t index)
        {
            if (index < sizeof(biosCallHandlerStr) / sizeof(biosCallHandlerStr[0])) {
                return biosCallHandlerStr[index];
            } else {
                return "INVALID";
            }
        }

        InstructionExecutionInfo softReset(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: softReset not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo registerRamReset(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: registerRamReset not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo halt(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: halt not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo stop(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: stop not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo intrWait(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: intrWait not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo vBlankIntrWait(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: vBankIntrWait not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }

        /*
        Output:
            r0: numerator / denominator
            r1: numerator % denominator
            r3: abs(numerator / denominator)
        */
        static InstructionExecutionInfo _div(uint32_t *const *const currentRegs, int32_t numerator, int32_t denominator)
        {
            if (denominator == 0) {
                std::cout << "WARNING: game attempted division by 0!" << std::endl;

                // Return something and pray that the game stops attempting suicide
                *currentRegs[regs::R0_OFFSET] = (numerator < 0) ? -1 : 1;
                *currentRegs[regs::R1_OFFSET] = static_cast<uint32_t>(numerator);
                *currentRegs[regs::R3_OFFSET] = 1;
            } else {
                int32_t div = numerator / denominator;

                *currentRegs[regs::R0_OFFSET] = static_cast<uint32_t>(div);
                // manually calculate remainder, because % isn't well defined for signed numbers across different platforms
                *currentRegs[regs::R1_OFFSET] = static_cast<uint32_t>(numerator - denominator * div);
                *currentRegs[regs::R3_OFFSET] = static_cast<uint32_t>(div < 0 ? -div : div);
            }

            //TODO proper time calculation
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo div(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            auto currentRegs = state->getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            return _div(currentRegs, numerator, denominator);
        }

        InstructionExecutionInfo divArm(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            auto currentRegs = state->getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            return _div(currentRegs, numerator, denominator);
        }

        InstructionExecutionInfo sqrt(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            r0 = static_cast<int32_t>(std::sqrt(r0));

            //TODO proper time calculation
            InstructionExecutionInfo info{0};
            return info;
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
        InstructionExecutionInfo arcTan(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
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

            std::cout << "WARNING: arcTan called, return format probably wrong!" << std::endl;

            //TODO proper time calculation
            InstructionExecutionInfo info{0};
            return info;
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
        InstructionExecutionInfo arcTan2(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            double x = convertFromQ1_14ToFP(static_cast<uint16_t>(r0 & 0x0000FFFF));
            double y = convertFromQ1_14ToFP(static_cast<uint16_t>(state->accessReg(regs::R1_OFFSET) & 0x0000FFFF));

            //TODO atan2 returns [-pi/2,pi/2] but we need [0, 2pi]... can we just make this interval transformation or is it calculated differently???
            double res = std::atan2(x, y) * 2 + M_PI;
            // Transform to integer interval [0, 0xFFFF]
            r0 = static_cast<uint32_t>(static_cast<uint16_t>((res * 0x0FFFF) / (2 * M_PI)));

            std::cout << "WARNING: arcTan2 called, return format probably wrong!" << std::endl;

            //TODO proper time calculation
            InstructionExecutionInfo info{0};
            return info;
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
        InstructionExecutionInfo cpuFastSet(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            const auto currentRegs = state->getCurrentRegs();
            //TODO we need to rewrite this code for memory access calculations or calculate it manually!
            Memory::MemoryRegion memReg;
            //TODO maybe sanity checks for memReg?
            //TODO what about mirroring?
            uint8_t *sourceAddr = state->memory.resolveAddr(*currentRegs[regs::R0_OFFSET], &info, memReg);
            uint8_t *destAddr = state->memory.resolveAddr(*currentRegs[regs::R1_OFFSET], &info, memReg);
            uint32_t length_mode = *currentRegs[regs::R2_OFFSET];

            // as we do more than 4 byte accesses this is no more safe and we need to exit now!
            if (info.hasCausedException) {
                return info;
            }

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

            return info;
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
        InstructionExecutionInfo cpuSet(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            //TODO we need to rewrite this code for memory access calculations or calculate it manually!
            Memory::MemoryRegion memReg;
            //TODO maybe sanity checks for memReg?
            //TODO what about mirroring?
            uint8_t *sourceAddr = state->memory.resolveAddr(*currentRegs[regs::R0_OFFSET], &info, memReg);
            uint8_t *destAddr = state->memory.resolveAddr(*currentRegs[regs::R1_OFFSET], &info, memReg);
            uint32_t length_mode = *currentRegs[regs::R2_OFFSET];

            // as we do more than 4 byte accesses this is no more safe and we need to exit now!
            if (info.hasCausedException) {
                return info;
            }

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

            return info;
        }

        /*
        Calculates the checksum of the BIOS ROM (by reading in 32bit units, and adding up these values). IRQ and FIQ are disabled during execution.
        The checksum is BAAE187Fh (GBA and GBA SP), or BAAE1880h (NDS/3DS in GBA mode, whereas the only difference is that the byte at [3F0Ch] is changed from 00h to 01h, otherwise the BIOS is 1:1 same as GBA BIOS, it does even include multiboot code).
        Parameters: None. Return: r0=Checksum.
        */
        InstructionExecutionInfo biosChecksum(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            state->accessReg(regs::R0_OFFSET) = 0x0BAAE18F;
            //TODO proper time calculation
            InstructionExecutionInfo info{0};
            return info;
        }

        /*
        SWI 0Eh (GBA) - BgAffineSet
        Used to calculate BG Rotation/Scaling parameters.
          r0   Pointer to Source Data Field with entries as follows:
                s32  Original data's center X coordinate (8bit fractional portion)
                s32  Original data's center Y coordinate (8bit fractional portion)
                s16  Display's center X coordinate
                s16  Display's center Y coordinate
                s16  Scaling ratio in X direction (8bit fractional portion)
                s16  Scaling ratio in Y direction (8bit fractional portion)
                u16  Angle of rotation (8bit fractional portion) Effective Range 0-FFFF
          r1   Pointer to Destination Data Field with entries as follows:
                s16  Difference in X coordinate along same line
                s16  Difference in X coordinate along next line
                s16  Difference in Y coordinate along same line
                s16  Difference in Y coordinate along next line
                s32  Start X coordinate
                s32  Start Y coordinate
          r2   Number of Calculations
        Return: No return value, Data written to destination address.

        For both Bg- and ObjAffineSet, Rotation angles are specified as 0-FFFFh (covering a range of 360 degrees), 
        however, the GBA BIOS recurses only the upper 8bit; the lower 8bit may contain a fractional portion, but it is ignored by the BIOS.
        */
        InstructionExecutionInfo bgAffineSet(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            //TODO do those read & writes count as non sequential?
            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];
            uint32_t iterationCount = *currentRegs[regs::R2_OFFSET];

            auto &m = state->memory;

            for (size_t i = 0; i < iterationCount; ++i) {
                common::math::mat<3, 3> scale{
                    {m.read32(sourceAddr, &info, i != 0) / 256.f, 0, 0},
                    {0, m.read32(sourceAddr + 4, &info, true) / 256.f, 0},
                    {0, 0, 1}};

                uint32_t off = i * 20;
                float ox = m.read32(off + sourceAddr, &info, true) / 256.f;
                float oy = m.read32(off + sourceAddr + 4, &info, true) / 256.f;
                float cx = m.read16(off + sourceAddr + 8, &info, true);
                float cy = m.read16(off + sourceAddr + 10, &info, true);
                float sx = m.read16(off + sourceAddr + 12, &info, true) / 256.f;
                float sy = m.read16(off + sourceAddr + 14, &info, true) / 256.f;
                float theta = (m.read32(sourceAddr + 16, &info, true) >> 8) / 128.f * M_PI;

                auto r = common::math::scale_matrix({sx, sy, 1}) *
                         common::math::rotation_around_matrix(theta, {0, 0, 1}, {cx - ox, cy - oy, 1});

                uint32_t dstOff = i * 16;
                m.write16(dstOff + destAddr, r[0][0] * 256, &info, i != 0);
                m.write16(dstOff + destAddr + 2, r[0][1] * 256, &info, true);
                m.write16(dstOff + destAddr + 4, r[1][0] * 256, &info, true);
                m.write16(dstOff + destAddr + 6, r[1][1] * 256, &info, true);
                m.write32(dstOff + destAddr + 8, r[0][2] * 256, &info, true);
                m.write32(dstOff + destAddr + 12, r[1][2] * 256, &info, true);
            }

            return info;
        }

        /*
        SWI 0Fh (GBA) - ObjAffineSet
        Calculates and sets the OBJ's affine parameters from the scaling ratio and angle of rotation.
        The affine parameters are calculated from the parameters set in Srcp.
        The four affine parameters are set every Offset bytes, starting from the Destp address.
        If the Offset value is 2, the parameters are stored contiguously. If the value is 8, they match the structure of OAM.
        When Srcp is arrayed, the calculation can be performed continuously by specifying Num.
          r0   Source Address, pointing to data structure as such:
                s16  Scaling ratio in X direction (8bit fractional portion)
                s16  Scaling ratio in Y direction (8bit fractional portion)
                u16  Angle of rotation (8bit fractional portion) Effective Range 0-FFFF
          r1   Destination Address, pointing to data structure as such:
                s16  Difference in X coordinate along same line
                s16  Difference in X coordinate along next line
                s16  Difference in Y coordinate along same line
                s16  Difference in Y coordinate along next line
          r2   Number of calculations
          r3   Offset in bytes for parameter addresses (2=continuous, 8=OAM)
        Return: No return value, Data written to destination address.

        For both Bg- and ObjAffineSet, Rotation angles are specified as 0-FFFFh (covering a range of 360 degrees), 
        however, the GBA BIOS recurses only the upper 8bit; the lower 8bit may contain a fractional portion, but it is ignored by the BIOS.
        */
        InstructionExecutionInfo objAffineSet(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];
            uint32_t iterationCount = *currentRegs[regs::R2_OFFSET];
            uint32_t diff = *currentRegs[regs::R2_OFFSET];

            auto &m = state->memory;
            //TODO do those read & writes count as non sequential?
            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            for (size_t i = 0; i < iterationCount; ++i) {
                uint32_t srcOff = i * 8;
                float sx = m.read16(srcOff + sourceAddr, &info, i != 0) / 256.f;
                float sy = m.read16(srcOff + sourceAddr + 2, &info, true) / 256.f;
                float theta = (m.read16(srcOff + sourceAddr + 4, &info, true) >> 8) / 128.f * M_PI;

                auto r = common::math::scale_matrix(sx, sy, 0) *
                         common::math::rotation_matrix(theta, {0, 0, 1});

                uint32_t destOff = diff * 4;
                m.write16(destOff + destAddr, r[0][0] * 256, &info, i != 0);
                m.write16(destOff + destAddr + diff, r[0][1] * 256, &info, true);
                m.write16(destOff + destAddr + diff * 2, r[1][0] * 256, &info, true);
                m.write16(destOff + destAddr + diff * 3, r[1][1] * 256, &info, true);
            }

            return info;
        }

        /*
        BitUnPack - SWI 10h (GBA/NDS7/NDS9/DSi7/DSi9)
        Used to increase the color depth of bitmaps or tile data. For example, to convert a 1bit monochrome font into 4bit or 8bit GBA tiles.
        The Unpack Info is specified separately, allowing to convert the same source data into different formats.
          r0  Source Address      (no alignment required)
          r1  Destination Address (must be 32bit-word aligned)
          r2  Pointer to UnPack information:
               16bit  Length of Source Data in bytes     (0-FFFFh)
               8bit   Width of Source Units in bits      (only 1,2,4,8 supported)
               8bit   Width of Destination Units in bits (only 1,2,4,8,16,32 supported)
               32bit  Data Offset (Bit 0-30), and Zero Data Flag (Bit 31)
              The Data Offset is always added to all non-zero source units.
              If the Zero Data Flag was set, it is also added to zero units.
        Data is written in 32bit units, Destination can be Wram or Vram. The size of unpacked data must be a multiple of 4 bytes.
        The width of source units (plus the offset) should not exceed the destination width.
        Return: No return value, Data written to destination address
        */
        InstructionExecutionInfo bitUnPack(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];
            uint32_t unpackFormatPtr = *currentRegs[regs::R2_OFFSET];

            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            uint16_t srcByteCount = state->memory.read16(unpackFormatPtr, &info, false);
            const uint8_t srcUnitWidth = state->memory.read8(unpackFormatPtr + 2, &info, true);
            const uint8_t destUnitWidth = state->memory.read8(unpackFormatPtr + 3, &info, true);
            uint32_t dataOffset = state->memory.read32(unpackFormatPtr + 4, &info, true);
            const bool zeroDataOff = dataOffset & (static_cast<uint32_t>(1) << 31);
            dataOffset &= 0x7FFFFFF;

            // data is should be written in 32 bit batches, therefore we have to buffer the decompressed data and keep track of the left space
            uint32_t writeBuf = 0;
            uint8_t writeBufOffset = 0;

            bool firstReadDone = false;
            bool firstWriteDone = false;

            for (; srcByteCount > 0; --srcByteCount) {
                uint8_t srcUnits = state->memory.read8(sourceAddr++, &info, firstReadDone);
                firstReadDone = true;

                // units of size < 8 will be concatenated so we need to extract them before storing
                for (uint8_t srcUnitBitsLeft = 8; srcUnitBitsLeft > 0; srcUnitBitsLeft -= srcUnitWidth) {
                    // extract unit: cut srcUnitWidth LSB bits
                    uint32_t unit = srcUnits & ((1 << srcUnitWidth) - 1);
                    // remove extracted unit
                    srcUnits >>= srcUnitWidth;

                    // apply offset
                    if (zeroDataOff || unit > 0) {
                        unit += dataOffset;
                    }

                    // cut to target size
                    unit &= ((1 << destUnitWidth) - 1);

                    // store extracted unit in write buf and update offset
                    writeBuf |= unit << writeBufOffset;
                    writeBufOffset += destUnitWidth;

                    // if there is no more space for another unit we have to flush our buffer
                    // flushing is also needed if this is the very last unit!
                    if (writeBufOffset + destUnitWidth > 32 || (srcByteCount == 1 && srcUnitBitsLeft <= srcUnitWidth)) {
                        state->memory.write32(destAddr, writeBuf, &info, firstWriteDone);
                        destAddr += 4;
                        writeBuf = 0;
                        writeBufOffset = 0;
                        firstWriteDone = true;
                    }
                }
            }

            return info;
        }

        /*
        LZ77UnCompReadNormalWrite8bit (Wram) - SWI 11h (GBA/NDS7/NDS9/DSi7/DSi9)
        LZ77UnCompReadNormalWrite16bit (Vram) - SWI 12h (GBA)
        LZ77UnCompReadByCallbackWrite8bit - SWI 01h (DSi7/DSi9)
        LZ77UnCompReadByCallbackWrite16bit - SWI 12h (NDS), SWI 02h or 19h (DSi)
        Expands LZ77-compressed data. The Wram function is faster, and writes in units of 8bits. 
        For the Vram function the destination must be halfword aligned, data is written in units of 16bits.
        CAUTION: Writing 16bit units to [dest-1] instead of 8bit units to [dest] means that reading from [dest-1] won't work, 
        ie. the "Vram" function works only with disp=001h..FFFh, but not with disp=000h.
        If the size of the compressed data is not a multiple of 4, please adjust it as much as possible by padding with 0.
        Align the source address to a 4-Byte boundary.
          r0  Source address, pointing to data as such:
               Data header (32bit)
                 Bit 0-3   Reserved
                 Bit 4-7   Compressed type (must be 1 for LZ77)
                 Bit 8-31  Size of decompressed data
               Repeat below. Each Flag Byte followed by eight Blocks.
               Flag data (8bit)
                 Bit 0-7   Type Flags for next 8 Blocks, MSB first
               Block Type 0 - Uncompressed - Copy 1 Byte from Source to Dest
                 Bit 0-7   One data byte to be copied to dest
               Block Type 1 - Compressed - Copy N+3 Bytes from Dest-Disp-1 to Dest
                 Bit 0-3   Disp MSBs
                 Bit 4-7   Number of bytes to copy (minus 3)
                 Bit 8-15  Disp LSBs
          r1  Destination address
          r2  Callback parameter        ;\for NDS/DSi "ReadByCallback" variants only
          r3  Callback structure        ;/(see Callback notes below)
        Return: No return value.
        */
        //TODO is the difference between writing in units of 8 bit vs 16 bit relevant?
        static InstructionExecutionInfo _LZ77UnComp(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            const uint32_t dataHeader = state->memory.read32(sourceAddr, &info, false);
            sourceAddr += 4;

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;
            uint32_t decompressedSize = (dataHeader >> 8) & 0x00FFFFFF;

            // Value should be 3 for run-length decompression
            if (compressedType != 1) {
                std::cerr << "ERROR: Invalid call of LZ77UnComp!" << std::endl;
                return info;
            }

            bool firstWriteDone = false;

            while (decompressedSize > 0) {
                const uint8_t typeBitset = state->memory.read8(sourceAddr++, &info, true);

                // process each block
                for (uint8_t i = 0; i < 8; ++i) {
                    bool type1 = (typeBitset >> (7 - i)) & 0x1;

                    if (type1) {
                        // Type 1 uses previously written data as lookup source
                        uint16_t type1Desc = state->memory.read16(sourceAddr, &info, true);
                        sourceAddr += 2;

                        uint16_t disp = (((type1Desc & 0x0F) << 8) | ((type1Desc >> 8) & 0x0FF)) + 1;
                        uint8_t n = ((type1Desc >> 4) & 0x0F) + 3;

                        // We read & write n bytes of uncompressed data
                        decompressedSize -= n;

                        // Copy N Bytes from Dest-Disp to Dest (+3 and - 1 already applied)
                        uint32_t readAddr = destAddr - disp;
                        while (n > 0) {
                            state->memory.write8(destAddr++, state->memory.read8(readAddr++, &info, true), &info, firstWriteDone);
                            firstWriteDone = true;
                        }
                    } else {
                        // Type 0 is one uncompressed byte of data
                        uint8_t data = state->memory.read8(sourceAddr++, &info, true);
                        --decompressedSize;
                        state->memory.write8(destAddr++, data, &info, firstWriteDone);
                        firstWriteDone = true;
                    }
                }
            }

            return info;
        }

        InstructionExecutionInfo LZ77UnCompWRAM(CPUState *state)
        {
            return _LZ77UnComp(state);
        }
        InstructionExecutionInfo LZ77UnCompVRAM(CPUState *state)
        {
            return _LZ77UnComp(state);
        }

        /*
        HuffUnCompReadNormal - SWI 13h (GBA)
        HuffUnCompReadByCallback - SWI 13h (NDS/DSi)
        The decoder starts in root node, the separate bits in the bitstream specify if the next node is node0 or node1, if that node is a data node, then the data is stored in memory, and the decoder is reset to the root node. The most often used data should be as close to the root node as possible. For example, the 4-byte string "Huff" could be compressed to 6 bits: 10-11-0-0, with root.0 pointing directly to data "f", and root.1 pointing to a child node, whose nodes point to data "H" and data "u".
        Data is written in units of 32bits, if the size of the compressed data is not a multiple of 4, please adjust it as much as possible by padding with 0.
        Align the source address to a 4Byte boundary.
          r0  Source Address, aligned by 4, pointing to:
               Data Header (32bit)
                 Bit0-3   Data size in bit units (normally 4 or 8)
                 Bit4-7   Compressed type (must be 2 for Huffman)
                 Bit8-31  24bit size of decompressed data in bytes
               Tree Size (8bit)
                 Bit0-7   Size of Tree Table/2-1 (ie. Offset to Compressed Bitstream)
               Tree Table (list of 8bit nodes, starting with the root node)
                Root Node and Non-Data-Child Nodes are:
                 Bit0-5   Offset to next child node,
                          Next child node0 is at (CurrentAddr AND NOT 1)+Offset*2+2
                          Next child node1 is at (CurrentAddr AND NOT 1)+Offset*2+2+1
                 Bit6     Node1 End Flag (1=Next child node is data)
                 Bit7     Node0 End Flag (1=Next child node is data)
                Data nodes are (when End Flag was set in parent node):
                 Bit0-7   Data (upper bits should be zero if Data Size is less than 8)
               Compressed Bitstream (stored in units of 32bits)
                 Bit0-31  Node Bits (Bit31=First Bit)  (0=Node0, 1=Node1)
          r1  Destination Address
          r2  Callback temp buffer      ;\for NDS/DSi "ReadByCallback" variants only
          r3  Callback structure        ;/(see Callback notes below)
        Return: No return value, Data written to destination address.
        */
        InstructionExecutionInfo huffUnComp(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            uint32_t dataHeader = state->memory.read32(sourceAddr, &info, false);
            sourceAddr += 4;

            uint32_t decompressedBits = ((dataHeader >> 8) & 0x00FFFFFF) * 8;
            //TODO do we need to add 1 for the correct size, else are 16bit data impossible and 0 bit data does not make sense
            const uint8_t dataSize = dataHeader & 0x0F;

            // data size should be a multiple of 4
            if (dataSize % 4) {
                std::cerr << "WARNING: huffman decompression data is not a multiple of 4 bit! PLS try to add 1 to the dataSize." << std::endl;
            }

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;

            // Value should be 2 for huffman
            if (compressedType != 2) {
                std::cerr << "ERROR: Invalid call of huffUnComp!" << std::endl;
                return info;
            }

            uint8_t treeSize = state->memory.read8(sourceAddr, &info, true);
            sourceAddr += 1;

            const uint32_t treeRoot = sourceAddr;
            sourceAddr += treeSize;

            // data is should be written in 32 bit batches, therefore we have to buffer the decompressed data and keep track of the left space
            uint32_t writeBuf = 0;
            uint8_t writeBufOffset = 0;

            // as bits needed for decompressions varies we need to keep track of bits left in the read buffer
            uint32_t readBuf = state->memory.read32(sourceAddr, &info, true);
            sourceAddr += 4;
            uint8_t readBufBitsLeft = 32;

            //TODO do we need to fix things if 32 % dataSize != 0?
            if (32 % dataSize) {
                std::cerr << "WARNING: decompressed huffman data might be misaligned, if not pls remove this warning and if so, well FML!" << std::endl;
            }

            bool firstWriteDone = false;

            for (; decompressedBits > 0; decompressedBits -= dataSize) {
                uint32_t currentParsingAddr = treeRoot;
                bool isDataNode = false;

                // Bit wise tree walk
                for (;;) {
                    // Probably non sequential
                    uint8_t node = state->memory.read8(currentParsingAddr, &info, false);

                    if (isDataNode) {
                        writeBuf |= (static_cast<uint32_t>(node) << writeBufOffset);
                        writeBufOffset += dataSize;
                        break;
                    }

                    // We have a parent node so lets check for the next node and if it is a data node
                    uint8_t offset = node & 0x1F;
                    bool isNode1EndFlag = (node >> 6) & 0x1;
                    bool isNode0EndFlag = (node >> 7) & 0x1;
                    --readBufBitsLeft;
                    bool decompressBit = (readBuf >> readBufBitsLeft) & 0x1;

                    isDataNode = decompressBit ? isNode1EndFlag : isNode0EndFlag;
                    //TODO the calculation of the next node might be wrong...
                    currentParsingAddr = (currentParsingAddr & (~static_cast<uint32_t>(1))) + static_cast<uint32_t>(offset) * 2 + (decompressBit ? 3 : 2);

                    // Fill empty read buffer again
                    if (readBufBitsLeft == 0) {
                        readBuf = state->memory.read32(sourceAddr, &info, true);
                        sourceAddr += 4;
                        readBufBitsLeft = 32;
                    }
                }

                // Is there is no more space left for decompressed data or we are done decompressing(only dataSize bits left) then we have to flush our buffer
                if (writeBufOffset + dataSize > 32 || decompressedBits == dataSize) {
                    state->memory.write32(destAddr, writeBuf, &info, firstWriteDone);
                    destAddr += 4;
                    // Reset buf state
                    writeBufOffset = 0;
                    writeBuf = 0;
                    firstWriteDone = true;
                }
            }

            return info;
        }

        /*
        RLUnCompReadNormalWrite8bit (Wram) - SWI 14h (GBA/NDS7/NDS9/DSi7/DSi9)
        RLUnCompReadNormalWrite16bit (Vram) - SWI 15h (GBA)
        RLUnCompReadByCallbackWrite16bit - SWI 15h (NDS7/NDS9/DSi7/DSi9)
        Expands run-length compressed data. The Wram function is faster, and writes in units of 8bits. 
        For the Vram function the destination must be halfword aligned, data is written in units of 16bits.
        If the size of the compressed data is not a multiple of 4, please adjust it as much as possible by padding with 0. Align the source address to a 4Byte boundary.
          r0  Source Address, pointing to data as such:
               Data header (32bit)
                 Bit 0-3   Reserved
                 Bit 4-7   Compressed type (must be 3 for run-length)
                 Bit 8-31  Size of decompressed data
               Repeat below. Each Flag Byte followed by one or more Data Bytes.
               Flag data (8bit)
                 Bit 0-6   Expanded Data Length (uncompressed N-1, compressed N-3)
                 Bit 7     Flag (0=uncompressed, 1=compressed)
               Data Byte(s) - N uncompressed bytes, or 1 byte repeated N times
          r1  Destination Address
          r2  Callback parameter        ;\for NDS/DSi "ReadByCallback" variants only
          r3  Callback structure        ;/(see Callback notes below)
        Return: No return value, Data written to destination address.
        */
        //TODO is the difference between writing in units of 8 bit vs 16 bit relevant?
        static InstructionExecutionInfo _rlUnComp(CPUState *state)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation
            InstructionExecutionInfo info{0};

            const uint32_t dataHeader = state->memory.read32(sourceAddr, &info, false);
            sourceAddr += 4;

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;
            uint32_t decompressedSize = (dataHeader >> 8) & 0x00FFFFFF;

            // Value should be 3 for run-length decompression
            if (compressedType != 3) {
                std::cerr << "ERROR: Invalid call of rlUnComp!" << std::endl;
                return info;
            }

            bool firstWriteDone = false;

            while (decompressedSize > 0) {
                uint8_t flagData = state->memory.read8(sourceAddr++, &info, true);

                bool compressed = (flagData >> 7) & 0x1;
                uint8_t decompressedDataLength = (flagData & 0x7F) + (compressed ? 3 : 1);

                if (decompressedSize < decompressedDataLength) {
                    std::cerr << "ERROR: underflow in rlUnComp!" << std::endl;
                    return info;
                }
                decompressedSize -= decompressedDataLength;

                uint8_t data = state->memory.read8(sourceAddr++, &info, true);

                // write read data byte N times for decompression
                for (; decompressedDataLength > 0; --decompressedDataLength) {
                    state->memory.write8(destAddr++, data, &info, firstWriteDone);
                    firstWriteDone = true;
                }
            }

            return info;
        }

        InstructionExecutionInfo RLUnCompWRAM(CPUState *state)
        {
            return _rlUnComp(state);
        }
        InstructionExecutionInfo RLUnCompVRAM(CPUState *state)
        {
            return _rlUnComp(state);
        }

        /*
        Diff8bitUnFilterWrite8bit (Wram) - SWI 16h (GBA/NDS9/DSi9)
        Diff8bitUnFilterWrite16bit (Vram) - SWI 17h (GBA)
        Diff16bitUnFilter - SWI 18h (GBA/NDS9/DSi9)
        These aren't actually real decompression functions, destination data will have exactly the same size as source data. However, assume a bitmap or wave form to contain a stream of increasing numbers such like 10..19, the filtered/unfiltered data would be:
          unfiltered:   10  11  12  13  14  15  16  17  18  19
          filtered:     10  +1  +1  +1  +1  +1  +1  +1  +1  +1
        In this case using filtered data (combined with actual compression algorithms) will obviously produce better compression results.
        Data units may be either 8bit or 16bit used with Diff8bit or Diff16bit functions respectively.
          r0  Source address (must be aligned by 4) pointing to data as follows:
               Data Header (32bit)
                 Bit 0-3   Data size (must be 1 for Diff8bit, 2 for Diff16bit)
                 Bit 4-7   Type (must be 8 for DiffFiltered)
                 Bit 8-31  24bit size after decompression
               Data Units (each 8bit or 16bit depending on used SWI function)
                 Data0          ;original data
                 Data1-Data0    ;difference data
                 Data2-Data1    ;...
                 Data3-Data2
                 ...
          r1  Destination address
        Return: No return value, Data written to destination address.
        */
        static InstructionExecutionInfo _diffUnFilter(CPUState *state, bool bits8)
        {
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            const auto currentRegs = state->getCurrentRegs();
            uint32_t srcAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation considering bit width!
            InstructionExecutionInfo cycleInfo{0};

            uint32_t info = state->memory.read32(srcAddr, &cycleInfo, false);
            srcAddr += 4;

            //TODO not sure if we need those... maybe for sanity checks
            /*uint8_t dataSize = info & 0x0F;
            uint8_t type = (info >> 4) & 0x0F;*/

            uint32_t size = (info >> 8) & 0x00FFFFFF;

            uint8_t addressInc = bits8 ? 1 : 2;

            uint16_t current = 0;
            bool firstWriteDone = false;
            do {
                uint16_t diff = bits8 ? state->memory.read8(srcAddr, &cycleInfo, false) : state->memory.read16(srcAddr, &cycleInfo, false);
                current += diff;
                bits8 ? state->memory.write8(destAddr, static_cast<uint8_t>(current & 0x0FF), &cycleInfo, firstWriteDone) : state->memory.write16(srcAddr, current, &cycleInfo, firstWriteDone);
                destAddr += addressInc;
                srcAddr += addressInc;
                firstWriteDone = true;
            } while (--size);

            return cycleInfo;
        }

        InstructionExecutionInfo diff8BitUnFilterWRAM(CPUState *state)
        {
            return _diffUnFilter(state, true);
        }
        InstructionExecutionInfo diff8BitUnFilterVRAM(CPUState *state)
        {
            return _diffUnFilter(state, true);
        }
        InstructionExecutionInfo diff16BitUnFilter(CPUState *state)
        {
            return _diffUnFilter(state, false);
        }

        InstructionExecutionInfo soundBiasChange(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundBiasChange not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundDriverInit(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundDriverInit not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundDriverMode(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundDriverMode not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundDriverMain(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundDirverMain not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundDriverVSync(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundDirverVSync not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundChannelClear(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundChannelClear not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo MIDIKey2Freq(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: MIDIKey2Freq not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo musicPlayerOpen(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: musicPlayerOpen not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo musicPlayerStart(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: musicPlayerStart not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo musicPlayerStop(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: musicPlayerStop not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo musicPlayerContinue(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: musicPlayerContinue not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo musicPlayerFadeOut(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: musicPlayerFadeOut not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo multiBoot(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: multiBoot not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo hardReset(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: hardReset not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo customHalt(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: customHalt not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundDriverVSyncOff(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: sourdDriverVSyncOff not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo soundDriverVSyncOn(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: soundDriverVSyncOn not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }
        InstructionExecutionInfo getJumpList(CPUState *state)
        {
            //TODO implement
            std::cout << "WARNING: getJumpList not yet implemented!" << std::endl;
            state->memory.setBiosReadState(Memory::BIOS_AFTER_SWI);
            InstructionExecutionInfo info{0};
            return info;
        }

    } // namespace swi
} // namespace gbaemu