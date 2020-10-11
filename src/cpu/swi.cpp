#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>

#include "cpu.hpp"
#include "cpu_state.hpp"
#include "lcd/lcd-controller.hpp"
#include "logging.hpp"
#include "math/math3d.hpp"
#include "regs.hpp"
#include "swi.hpp"

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

        void callBiosCodeSWIHandler(CPU *cpu)
        {
            /*
            How BIOS Processes SWIs
            SWIs can be called from both within THUMB and ARM mode. In ARM mode, only the upper 8bit of the 24bit comment field are interpreted.
            Each time when calling a BIOS function 4 words (SPSR, R11, R12, R14) are saved on Supervisor stack (_svc). Once it has saved that data, the SWI handler switches into System mode, so that all further stack operations are using user stack.
            In some cases the BIOS may allow interrupts to be executed from inside of the SWI procedure. If so, and if the interrupt handler calls further SWIs, then care should be taken that the Supervisor Stack does not overflow.
            */

            // Save the current CPSR register value into SPSR_svc
            auto svcRegs = cpu->state.getModeRegs(CPUState::SupervisorMode);
            *(svcRegs[regs::SPSR_OFFSET]) = cpu->state.getCurrentCPSR();
            // Save PC to LR_svc
            *(svcRegs[regs::LR_OFFSET]) = cpu->state.getCurrentPC() + (cpu->state.getFlag<cpsr_flags::THUMB_STATE>() ? 2 : 4);

            // Ensure that the CPSR represents that we are in ARM mode again
            // Clear all flags & enforce supervisor mode
            // Also disable interrupts
            cpu->state.clearFlags();
            cpu->state.setFlag<cpsr_flags::IRQ_DISABLE>(true);
            cpu->state.setCPUMode(0b010011);

            // Offset to the swi routine
            *svcRegs[regs::PC_OFFSET] = Memory::BIOS_SWI_HANDLER_OFFSET;

            cpu->state.cpuInfo.forceBranch = true;
        }

        void softReset(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: softReset not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void registerRamReset(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: registerRamReset not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void stop(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: stop not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }

        void halt(CPU *cpu)
        {
            /*
                SWI 02h (GBA) or SWI 06h (NDS7/NDS9/DSi7/DSi9) - Halt

                Halts the CPU until an interrupt request occurs. The CPU is switched into low-power mode, all other 
                circuits (video, sound, timers, serial, keypad, system clock) are kept operating.
                Halt mode is terminated when any enabled interrupts are requested, that is when (IE AND IF) is not 
                zero, the GBA locks up if that condition doesn't get true. However, the state of CPUs IRQ disable 
                bit in CPSR register, and the IME register are don't care, Halt passes through even if either one 
                has disabled interrupts.
                On GBA and NDS7/DSi7, Halt is implemented by writing to HALTCNT, Port 4000301h. On NDS9/DSi9, Halt 
                is implemted by writing to System Control Coprocessor (mov p15,0,c7,c0,4,r0 opcode), this opcode hangs if IME=0.
                No parameters, no return value.
                (GBA/NDS7/DSi7: all registers unchanged, NDS9/DSi9: R0 destroyed)
            */

            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);

            cpu->state.cpuInfo.haltCPU = true;
            // load IE
            cpu->state.cpuInfo.haltCondition = cpu->state.memory.ioHandler.internalRead16(InterruptHandler::INTERRUPT_CONTROL_REG_ADDR);
        }
        void intrWait(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);

            // The function forcefully sets IME=1
            cpu->state.memory.ioHandler.externalWrite8(InterruptHandler::INTERRUPT_CONTROL_REG_ADDR + 8, 0x1);

            cpu->state.cpuInfo.haltCondition = cpu->state.accessReg(regs::R1_OFFSET);

            if (cpu->state.accessReg(regs::R0_OFFSET)) {
                // r0 = 1 = Discard old flags, wait until a NEW flag becomes set
                // Discard current requests! : write into IF
                cpu->state.cpuInfo.haltCPU = true;

                //TODO only discard in R1 selected ones or all??
                cpu->state.memory.ioHandler.externalWrite16(InterruptHandler::INTERRUPT_CONTROL_REG_ADDR + 2, cpu->state.cpuInfo.haltCondition);
            } else {
                //0=Return immediately if an old flag was already set
                // check if condition is currently satisfied and if so return else cause halt
                cpu->state.cpuInfo.haltCPU = !cpu->irqHandler.checkForHaltCondition(cpu->state.cpuInfo.haltCondition);
            }
        }
        void vBlankIntrWait(CPU *cpu)
        {
            cpu->state.accessReg(regs::R0_OFFSET) = 0;
            cpu->state.accessReg(regs::R1_OFFSET) = 1;
            intrWait(cpu);
        }

        /*
        Output:
            r0: numerator / denominator
            r1: numerator % denominator
            r3: abs(numerator / denominator)
        */
        static void _div(InstructionExecutionInfo &info, uint32_t *const *const currentRegs, int32_t numerator, int32_t denominator)
        {
            if (denominator == 0) {
                LOG_SWI(std::cout << "WARNING: game attempted division by 0!" << std::endl;);

                // Return something and pray that the game stops attempting suicide
                *currentRegs[regs::R0_OFFSET] = (numerator < 0) ? -1 : 1;
                *currentRegs[regs::R1_OFFSET] = static_cast<uint32_t>(numerator);
                *currentRegs[regs::R3_OFFSET] = 1;
            } else if (numerator == static_cast<int32_t>(0x80000000) && denominator == static_cast<int32_t>(0xFFFFFFFF)) {
                *currentRegs[regs::R0_OFFSET] = 0x80000000;
                *currentRegs[regs::R1_OFFSET] = 0;
                *currentRegs[regs::R3_OFFSET] = 0x80000000;
            } else {
                /* The standard way to do it. */
                div_t result = std::div(numerator, denominator);

                *currentRegs[regs::R0_OFFSET] = result.quot;
                *currentRegs[regs::R1_OFFSET] = result.rem;
                *currentRegs[regs::R3_OFFSET] = std::abs(result.quot);
            }

            //TODO proper time calculation
        }
        void div(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            auto currentRegs = cpu->state.getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            _div(cpu->state.cpuInfo, currentRegs, numerator, denominator);
        }

        void divArm(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            auto currentRegs = cpu->state.getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            _div(cpu->state.cpuInfo, currentRegs, numerator, denominator);
        }

        void sqrt(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            uint32_t &r0 = cpu->state.accessReg(regs::R0_OFFSET);
            r0 = static_cast<int32_t>(std::sqrt(r0));

            //TODO proper time calculation
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
        void arcTan(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);

            uint32_t &r0 = cpu->state.accessReg(regs::R0_OFFSET);
            uint32_t &r1 = cpu->state.accessReg(regs::R1_OFFSET);
            uint32_t &r2 = cpu->state.accessReg(regs::R2_OFFSET);

            int32_t i = r0;
            int32_t a = -((i * i) >> 14);
            int32_t b = ((0xA9 * a) >> 14) + 0x390;
            b = ((b * a) >> 14) + 0x91C;
            b = ((b * a) >> 14) + 0xFB6;
            b = ((b * a) >> 14) + 0x16AA;
            b = ((b * a) >> 14) + 0x2081;
            b = ((b * a) >> 14) + 0x3651;
            b = ((b * a) >> 14) + 0xA2F9;
            r0 = (i * b) >> 16;

            if (a)
                r1 = a;

            if (b)
                r2 = b;

            LOG_SWI(std::cout << "WARNING: arcTan called, return format probably wrong!" << std::endl;);

            //TODO proper time calculation
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
        void arcTan2(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            uint32_t &r0 = cpu->state.accessReg(regs::R0_OFFSET);
            double x = convertFromQ1_14ToFP(static_cast<uint16_t>(r0 & 0x0000FFFF));
            double y = convertFromQ1_14ToFP(static_cast<uint16_t>(cpu->state.accessReg(regs::R1_OFFSET) & 0x0000FFFF));

            //TODO atan2 returns [-pi/2,pi/2] but we need [0, 2pi]... can we just make this interval transformation or is it calculated differently???
            double res = std::atan2(x, y) * 2 + M_PI;
            // Transform to integer interval [0, 0xFFFF]
            r0 = static_cast<uint32_t>(static_cast<uint16_t>((res * 0x0FFFF) / (2 * M_PI)));

            LOG_SWI(std::cout << "WARNING: arcTan2 called, return format probably wrong!" << std::endl;);

            //TODO proper time calculation
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
        void cpuFastSet(CPU *cpu)
        {
            callBiosCodeSWIHandler(cpu);
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
        void cpuSet(CPU *cpu)
        {
            callBiosCodeSWIHandler(cpu);
        }

        /*
        Calculates the checksum of the BIOS ROM (by reading in 32bit units, and adding up these values). IRQ and FIQ are disabled during execution.
        The checksum is BAAE187Fh (GBA and GBA SP), or BAAE1880h (NDS/3DS in GBA mode, whereas the only difference is that the byte at [3F0Ch] is changed from 00h to 01h, otherwise the BIOS is 1:1 same as GBA BIOS, it does even include multiboot code).
        Parameters: None. Return: r0=Checksum.
        */
        void biosChecksum(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            cpu->state.accessReg(regs::R0_OFFSET) = 0x0BAAE18F;
            //TODO proper time calculation
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
        void bgAffineSet(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            //TODO do those read & writes count as non sequential?
            //TODO proper time calculation

            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];
            uint32_t iterationCount = *currentRegs[regs::R2_OFFSET];

            auto &m = cpu->state.memory;

            for (uint32_t i = 0; i < iterationCount; ++i) {
                uint32_t off = i * 20;
                float ox = m.read32(off + sourceAddr, cpu->state.cpuInfo, true) / 256.f;
                float oy = m.read32(off + sourceAddr + 4, cpu->state.cpuInfo, true) / 256.f;
                float cx = m.read16(off + sourceAddr + 8, cpu->state.cpuInfo, true);
                float cy = m.read16(off + sourceAddr + 10, cpu->state.cpuInfo, true);
                float sx = m.read16(off + sourceAddr + 12, cpu->state.cpuInfo, true) / 256.f;
                float sy = m.read16(off + sourceAddr + 14, cpu->state.cpuInfo, true) / 256.f;
                float theta = (m.read32(sourceAddr + 16, cpu->state.cpuInfo, true) >> 8) / 128.f * M_PI;

                /*
                common::math::real_t ox = fixedToFloat<uint32_t, 8, 19>(m.read32(off + sourceAddr,     cpu->state.cpuInfo, true));
                common::math::real_t oy = fixedToFloat<uint32_t, 8, 19>(m.read32(off + sourceAddr + 4, cpu->state.cpuInfo, true));
                common::math::real_t cx = static_cast<common::math::real_t>(m.read16(off + sourceAddr + 8, cpu->state.cpuInfo, true));
                common::math::real_t cy = static_cast<common::math::real_t>(m.read16(off + sourceAddr + 10, cpu->state.cpuInfo, true));
                common::math::real_t sx = fixedToFloat<uint16_t, 8, 7>(m.read16(off + sourceAddr + 12, cpu->state.cpuInfo, true));
                common::math::real_t sy = fixedToFloat<uint16_t, 8, 7>(m.read16(off + sourceAddr + 14, cpu->state.cpuInfo, true));
                 */

                /* F* THIS. This is taken from mgba. */
                common::math::real_t a, b, c, d, rx, ry;
                a = d = cosf(theta);
                b = c = sinf(theta);
                // Scale
                a *= sx;
                b *= -sx;
                c *= sy;
                d *= sy;
                // Translate
                rx = ox - (a * cx + b * cy);
                ry = oy - (c * cx + d * cy);

                uint32_t dstOff = i * 16;
                m.write16(dstOff + destAddr, gbaemu::floatToFixed<uint16_t, 8, 7, common::math::real_t>(a), cpu->state.cpuInfo, i != 0);
                m.write16(dstOff + destAddr + 2, gbaemu::floatToFixed<uint16_t, 8, 7, common::math::real_t>(b), cpu->state.cpuInfo, true);
                m.write16(dstOff + destAddr + 4, gbaemu::floatToFixed<uint16_t, 8, 7, common::math::real_t>(c), cpu->state.cpuInfo, true);
                m.write16(dstOff + destAddr + 6, gbaemu::floatToFixed<uint16_t, 8, 7, common::math::real_t>(d), cpu->state.cpuInfo, true);
                m.write32(dstOff + destAddr + 8, gbaemu::floatToFixed<uint32_t, 8, 19, common::math::real_t>(rx), cpu->state.cpuInfo, true);
                m.write32(dstOff + destAddr + 12, gbaemu::floatToFixed<uint32_t, 8, 19, common::math::real_t>(ry), cpu->state.cpuInfo, true);
            }
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
        void objAffineSet(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];
            const uint32_t iterationCount = *currentRegs[regs::R2_OFFSET];
            uint32_t diff = *currentRegs[regs::R3_OFFSET];

            auto &m = cpu->state.memory;
            //TODO do those read & writes count as non sequential?
            //TODO proper time calculation

            for (uint32_t i = 0; i < iterationCount; ++i) {
                uint32_t srcOff = i * 8;
                //float sx = m.read16(sourceAddr, cpu->state.cpuInfo, i != 0) / 256.f;
                //float sy = m.read16(sourceAddr + 2, cpu->state.cpuInfo, true) / 256.f;
                uint16_t orgTheta = m.read16(sourceAddr + 4, cpu->state.cpuInfo, true);
                uint16_t orgSx = m.read16(sourceAddr, cpu->state.cpuInfo, i != 0);
                uint16_t orgSy = m.read16(sourceAddr + 2, cpu->state.cpuInfo, i != 0);

                float theta = (orgTheta >> 8) / 128.f * M_PI;

                common::math::real_t sx = fixedToFloat<uint16_t, 8, 7>(orgSx);
                common::math::real_t sy = fixedToFloat<uint16_t, 8, 7>(orgSy);

                sourceAddr += 8;

                common::math::real_t a, b, c, d;
                a = d = cosf(theta);
                b = c = sinf(theta);
                // Scale
                a *= sx;
                b *= -sx;
                c *= sy;
                d *= sy;

                m.write16(destAddr, floatToFixed<uint16_t, 8, 7>(a), cpu->state.cpuInfo, i != 0);
                m.write16(destAddr + diff, floatToFixed<uint16_t, 8, 7>(b), cpu->state.cpuInfo, true);
                m.write16(destAddr + diff * 2, floatToFixed<uint16_t, 8, 7>(c), cpu->state.cpuInfo, true);
                m.write16(destAddr + diff * 3, floatToFixed<uint16_t, 8, 7>(d), cpu->state.cpuInfo, true);
                destAddr += diff * 4;
            }
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
        void bitUnPack(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];
            uint32_t unpackFormatPtr = *currentRegs[regs::R2_OFFSET];

            //TODO proper time calculation

            uint16_t srcByteCount = cpu->state.memory.read16(unpackFormatPtr, cpu->state.cpuInfo, false);
            const uint8_t srcUnitWidth = cpu->state.memory.read8(unpackFormatPtr + 2, cpu->state.cpuInfo, true);
            const uint8_t destUnitWidth = cpu->state.memory.read8(unpackFormatPtr + 3, cpu->state.cpuInfo, true);
            uint32_t dataOffset = cpu->state.memory.read32(unpackFormatPtr + 4, cpu->state.cpuInfo, true);
            const bool zeroDataOff = dataOffset & (static_cast<uint32_t>(1) << 31);
            dataOffset &= 0x7FFFFFF;

            // data is should be written in 32 bit batches, therefore we have to buffer the decompressed data and keep track of the left space
            uint32_t writeBuf = 0;
            uint8_t writeBufOffset = 0;

            bool firstReadDone = false;
            bool firstWriteDone = false;

            for (; srcByteCount > 0; --srcByteCount) {
                uint8_t srcUnits = cpu->state.memory.read8(sourceAddr++, cpu->state.cpuInfo, firstReadDone);
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
                        cpu->state.memory.write32(destAddr, writeBuf, cpu->state.cpuInfo, firstWriteDone);
                        destAddr += 4;
                        writeBuf = 0;
                        writeBufOffset = 0;
                        firstWriteDone = true;
                    }
                }
            }
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
        static void _LZ77UnComp(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation

            const uint32_t dataHeader = cpu->state.memory.read32(sourceAddr, cpu->state.cpuInfo, false);
            sourceAddr += 4;

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;
            int32_t decompressedSize = (dataHeader >> 8) & 0x00FFFFFF;

            // Value should be 3 for run-length decompression
            if (compressedType != 1) {
                LOG_SWI(std::cout << "ERROR: Invalid call of LZ77UnComp!" << std::endl;);
            }

            bool firstWriteDone = false;

            while (decompressedSize > 0) {
                const uint8_t typeBitset = cpu->state.memory.read8(sourceAddr++, cpu->state.cpuInfo, true);

                // process each block
                for (uint8_t i = 0; i < 8; ++i) {
                    bool type1 = (typeBitset >> (7 - i)) & 0x1;

                    if (type1) {
                        // Type 1 uses previously written data as lookup source
                        uint16_t type1Desc = cpu->state.memory.read16(sourceAddr, cpu->state.cpuInfo, true);
                        sourceAddr += 2;

                        uint16_t disp = (((type1Desc & 0x0F) << 8) | ((type1Desc >> 8) & 0x0FF)) + 1;
                        uint8_t n = ((type1Desc >> 4) & 0x0F) + 3;

                        // We read & write n bytes of uncompressed data
                        decompressedSize -= n;

                        // Copy N Bytes from Dest-Disp to Dest (+3 and - 1 already applied)
                        uint32_t readAddr = destAddr - disp;
                        for (; n > 0; --n) {
                            cpu->state.memory.write8(destAddr++, cpu->state.memory.read8(readAddr++, cpu->state.cpuInfo, true), cpu->state.cpuInfo, firstWriteDone);
                            firstWriteDone = true;
                        }
                    } else {
                        // Type 0 is one uncompressed byte of data
                        uint8_t data = cpu->state.memory.read8(sourceAddr++, cpu->state.cpuInfo, true);
                        --decompressedSize;
                        cpu->state.memory.write8(destAddr++, data, cpu->state.cpuInfo, firstWriteDone);
                        firstWriteDone = true;
                    }
                }
            }
        }

        void LZ77UnCompWRAM(CPU *cpu)
        {
            _LZ77UnComp(cpu);
        }
        void LZ77UnCompVRAM(CPU *cpu)
        {
            _LZ77UnComp(cpu);
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
        void huffUnComp(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation

            uint32_t dataHeader = cpu->state.memory.read32(sourceAddr, cpu->state.cpuInfo, false);
            sourceAddr += 4;

            int32_t decompressedBits = ((dataHeader >> 8) & 0x00FFFFFF) * 8;
            //TODO do we need to add 1 for the correct size, else are 16bit data impossible and 0 bit data does not make sense
            const uint8_t dataSize = dataHeader & 0x0F;

            // data size should be a multiple of 4
            if (dataSize % 4) {
                LOG_SWI(std::cout << "WARNING: huffman decompression data is not a multiple of 4 bit! PLS try to add 1 to the dataSize." << std::endl;);
            }

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;

            // Value should be 2 for huffman
            if (compressedType != 2) {
                LOG_SWI(std::cout << "ERROR: Invalid call of huffUnComp!" << std::endl;);
            }

            uint8_t treeSize = cpu->state.memory.read8(sourceAddr, cpu->state.cpuInfo, true);
            sourceAddr += 1;

            const uint32_t treeRoot = sourceAddr;
            sourceAddr += treeSize;

            // data is should be written in 32 bit batches, therefore we have to buffer the decompressed data and keep track of the left space
            uint32_t writeBuf = 0;
            uint8_t writeBufOffset = 0;

            // as bits needed for decompressions varies we need to keep track of bits left in the read buffer
            uint32_t readBuf = cpu->state.memory.read32(sourceAddr, cpu->state.cpuInfo, true);
            sourceAddr += 4;
            uint8_t readBufBitsLeft = 32;

            //TODO do we need to fix things if 32 % dataSize != 0?
            if (32 % dataSize) {
                LOG_SWI(std::cout << "WARNING: decompressed huffman data might be misaligned, if not pls remove this warning and if so, well FML!" << std::endl;);
            }

            bool firstWriteDone = false;

            for (; decompressedBits > 0; decompressedBits -= dataSize) {
                uint32_t currentParsingAddr = treeRoot;
                bool isDataNode = false;

                // Bit wise tree walk
                for (;;) {
                    // Probably non sequential
                    uint8_t node = cpu->state.memory.read8(currentParsingAddr, cpu->state.cpuInfo, false);

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
                        readBuf = cpu->state.memory.read32(sourceAddr, cpu->state.cpuInfo, true);
                        sourceAddr += 4;
                        readBufBitsLeft = 32;
                    }
                }

                // Is there is no more space left for decompressed data or we are done decompressing(only dataSize bits left) then we have to flush our buffer
                if (writeBufOffset + dataSize > 32 || decompressedBits == dataSize) {
                    cpu->state.memory.write32(destAddr, writeBuf, cpu->state.cpuInfo, firstWriteDone);
                    destAddr += 4;
                    // Reset buf state
                    writeBufOffset = 0;
                    writeBuf = 0;
                    firstWriteDone = true;
                }
            }
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
        static void _rlUnComp(CPU *cpu)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation

            const uint32_t dataHeader = cpu->state.memory.read32(sourceAddr, cpu->state.cpuInfo, false);
            sourceAddr += 4;

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;
            int32_t decompressedSize = (dataHeader >> 8) & 0x00FFFFFF;

            // Value should be 3 for run-length decompression
            if (compressedType != 3) {
                LOG_SWI(std::cout << "ERROR: Invalid call of rlUnComp!" << std::endl;);
            }

            bool firstWriteDone = false;

            while (decompressedSize > 0) {
                uint8_t flagData = cpu->state.memory.read8(sourceAddr++, cpu->state.cpuInfo, true);

                bool compressed = (flagData >> 7) & 0x1;
                uint8_t decompressedDataLength = (flagData & 0x7F) + (compressed ? 3 : 1);

                if (decompressedSize < decompressedDataLength) {
                    LOG_SWI(std::cout << "ERROR: underflow in rlUnComp!" << std::endl;);
                }
                decompressedSize -= decompressedDataLength;

                uint8_t data = cpu->state.memory.read8(sourceAddr++, cpu->state.cpuInfo, true);

                // write read data byte N times for decompression
                for (; decompressedDataLength > 0; --decompressedDataLength) {
                    cpu->state.memory.write8(destAddr++, data, cpu->state.cpuInfo, firstWriteDone);
                    firstWriteDone = true;
                }
            }
        }

        void RLUnCompWRAM(CPU *cpu)
        {
            _rlUnComp(cpu);
        }
        void RLUnCompVRAM(CPU *cpu)
        {
            _rlUnComp(cpu);
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
        static void _diffUnFilter(CPU *cpu, bool bits8)
        {
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
            const auto currentRegs = cpu->state.getCurrentRegs();
            uint32_t srcAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            //TODO proper time calculation considering bit width!

            uint32_t header = cpu->state.memory.read32(srcAddr, cpu->state.cpuInfo, false);
            srcAddr += 4;

            //TODO not sure if we need those... maybe for sanity checks
            /*uint8_t dataSize = header & 0x0F;
            uint8_t type = (header >> 4) & 0x0F;*/

            uint32_t size = (header >> 8) & 0x00FFFFFF;

            uint8_t addressInc = bits8 ? 1 : 2;

            uint16_t current = 0;
            bool firstWriteDone = false;
            do {
                uint16_t diff = bits8 ? cpu->state.memory.read8(srcAddr, cpu->state.cpuInfo, false) : cpu->state.memory.read16(srcAddr, cpu->state.cpuInfo, false);
                current += diff;
                bits8 ? cpu->state.memory.write8(destAddr, static_cast<uint8_t>(current & 0x0FF), cpu->state.cpuInfo, firstWriteDone) : cpu->state.memory.write16(srcAddr, current, cpu->state.cpuInfo, firstWriteDone);
                destAddr += addressInc;
                srcAddr += addressInc;
                firstWriteDone = true;
            } while (--size);
        }

        void diff8BitUnFilterWRAM(CPU *cpu)
        {
            _diffUnFilter(cpu, true);
        }
        void diff8BitUnFilterVRAM(CPU *cpu)
        {
            _diffUnFilter(cpu, true);
        }
        void diff16BitUnFilter(CPU *cpu)
        {
            _diffUnFilter(cpu, false);
        }

        void soundBiasChange(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundBiasChange not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundDriverInit(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundDriverInit not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundDriverMode(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundDriverMode not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundDriverMain(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundDirverMain not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundDriverVSync(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundDirverVSync not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundChannelClear(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundChannelClear not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void MIDIKey2Freq(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: MIDIKey2Freq not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void musicPlayerOpen(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: musicPlayerOpen not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void musicPlayerStart(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: musicPlayerStart not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void musicPlayerStop(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: musicPlayerStop not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void musicPlayerContinue(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: musicPlayerContinue not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void musicPlayerFadeOut(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: musicPlayerFadeOut not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void multiBoot(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: multiBoot not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void hardReset(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: hardReset not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void customHalt(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: customHalt not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundDriverVSyncOff(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: sourdDriverVSyncOff not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void soundDriverVSyncOn(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: soundDriverVSyncOn not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }
        void getJumpList(CPU *cpu)
        {
            //TODO implement
            LOG_SWI(std::cout << "WARNING: getJumpList not yet implemented!" << std::endl;);
            cpu->state.memory.setBiosState(Bios::BIOS_AFTER_SWI);
        }

    } // namespace swi
} // namespace gbaemu