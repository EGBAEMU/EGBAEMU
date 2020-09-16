
#include "dma.hpp"
#include "cpu/cpu.hpp"
#include "interrupts.hpp"
#include "memory.hpp"
#include "util.hpp"

#include <functional>
#include <iostream>

namespace gbaemu
{

    const char *DMA::countTypeToStr(AddrCntType updateKind)
    {
        switch (updateKind) {
            STRINGIFY_CASE_ID(INCREMENT_RELOAD);
            STRINGIFY_CASE_ID(INCREMENT);
            STRINGIFY_CASE_ID(DECREMENT);
            STRINGIFY_CASE_ID(FIXED);
        }
        return "";
    }

    uint8_t DMA::read8FromReg(uint32_t offset)
    {
        return *(offset + reinterpret_cast<uint8_t *>(&regs));
    }

    void DMA::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        //std::cout << "DMA" << std::dec << static_cast<uint32_t>(channel) << ": write to offset: 0x" << std::hex << offset << " value: 0x" << std::hex << static_cast<uint32_t>(value) << std::endl;
    }

    DMA::DMA(DMAChannel channel, CPU *cpu) : channel(channel), state(IDLE), memory(cpu->state.memory), irqHandler(cpu->irqHandler)
    {
        memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                DMA_BASE_ADDRESSES[channel],
                DMA_BASE_ADDRESSES[channel] + sizeof(regs) - 1,
                std::bind(&DMA::read8FromReg, this, std::placeholders::_1),
                std::bind(&DMA::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&DMA::read8FromReg, this, std::placeholders::_1),
                std::bind(&DMA::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
    }

    InstructionExecutionInfo DMA::step()
    {
        InstructionExecutionInfo info{0};

        // FSM actions
        switch (state) {
            case IDLE:
                if (extractRegValues()) {
                    state = conditionSatisfied() ? STARTED : WAITING_PAUSED;
                    std::cout << "INFO: Registered DMA" << std::dec << static_cast<uint32_t>(channel) << " transfer request." << std::endl;
                    std::cout << "      Source Addr: 0x" << std::hex << srcAddr << " Type: " << countTypeToStr(srcCnt) << std::endl;
                    std::cout << "      Dest Addr:   0x" << std::hex << destAddr << " Type: " << countTypeToStr(dstCnt) << std::endl;
                    std::cout << "      Words: 0x" << std::hex << count << std::endl;
                    std::cout << "      Repeat: " << repeat << std::endl;
                    std::cout << "      GamePak DRQ: " << gamePakDRQ << std::endl;
                    std::cout << "      32 bit mode: " << width32Bit << std::endl;
                    std::cout << "      IRQ on end: " << irqOnEnd << std::endl;
                }
                break;

            case REPEAT: {
                if (checkForUserAbort()) {
                    break;
                }

                //TODO reload only after condition is satisfied???
                //TODO is the src address kept?

                if (dstCnt == INCREMENT_RELOAD) {
                    destAddr = le(regs.destAddr);
                }

                fetchCount();

                state = WAITING_PAUSED;
                break;
            }

            case WAITING_PAUSED: {
                if (checkForUserAbort()) {
                    break;
                }
                state = conditionSatisfied() ? STARTED : WAITING_PAUSED;
                break;
            }

            case STARTED: {
                if (checkForUserAbort()) {
                    break;
                }
                info.dmaExecutes = true;

                state = SEQ_COPY;

                if (width32Bit) {
                    uint32_t data = memory.read32(srcAddr, &info, false);
                    memory.write32(destAddr, data, &info, false);
                } else {
                    uint16_t data = memory.read16(srcAddr, &info, false);
                    memory.write16(destAddr, data, &info, false);
                }

                --count;

                updateAddr(srcAddr, srcCnt);
                updateAddr(destAddr, dstCnt);

                break;
            }

            case SEQ_COPY: {
                if (checkForUserAbort()) {
                    break;
                }
                info.dmaExecutes = true;

                if (count == 0) {
                    state = DONE;
                } else {
                    if (width32Bit) {
                        uint32_t data = memory.read32(srcAddr, &info, true);
                        memory.write32(destAddr, data, &info, true);
                    } else {
                        uint16_t data = memory.read16(srcAddr, &info, true);
                        memory.write16(destAddr, data, &info, true);
                    }

                    --count;

                    updateAddr(srcAddr, srcCnt);
                    updateAddr(destAddr, dstCnt);
                }
                break;
            }

            case DONE: {
                if (repeat) {
                    state = REPEAT;
                } else {
                    // return to idle state
                    state = IDLE;

                    // Clear enable bit to indicate that we are done!
                    regs.cntReg &= ~le(DMA_CNT_REG_EN_MASK);

                    if (irqOnEnd) {
                        irqHandler.setInterrupt(static_cast<InterruptHandler::InterruptType>(InterruptHandler::InterruptType::DMA_0 + channel));
                    }
                }

                break;
            }
        }

        return info;
    }

    bool DMA::checkForUserAbort()
    { // Still enabled? else go back to IDLE
        if ((le(regs.cntReg) & DMA_CNT_REG_EN_MASK) == 0) {
            state = IDLE;
            return true;
        }
        return false;
    }

    void DMA::updateAddr(uint32_t &addr, AddrCntType updateKind) const
    {
        switch (updateKind) {
            case INCREMENT_RELOAD:
            case INCREMENT:
                addr += width32Bit ? 4 : 2;
                break;
            case DECREMENT:
                addr -= width32Bit ? 4 : 2;
                break;
            case FIXED:
                // Nothing to do here
                break;
        }
    }

    bool DMA::extractRegValues()
    {
        const uint16_t controlReg = le(regs.cntReg);
        bool enable = controlReg & DMA_CNT_REG_EN_MASK;

        if (enable) {
            // Extract remaining values
            repeat = controlReg & DMA_CNT_REG_REPEAT_MASK;
            gamePakDRQ = controlReg & DMA_CNT_REG_DRQ_MASK;
            irqOnEnd = controlReg & DMA_CNT_REG_IRQ_MASK;
            width32Bit = controlReg & DMA_CNT_REG_TYPE_MASK;
            srcCnt = static_cast<AddrCntType>((controlReg & DMA_CNT_REG_SRC_ADR_CNT_MASK) >> DMA_CNT_REG_SRC_ADR_CNT_OFF);
            dstCnt = static_cast<AddrCntType>((controlReg & DMA_CNT_REG_DST_ADR_CNT_MASK) >> DMA_CNT_REG_DST_ADR_CNT_OFF);
            condition = static_cast<StartCondition>((controlReg & DMA_CNT_REG_TIMING_MASK) >> DMA_CNT_REG_TIMING_OFF);

            // Mask out ignored bits
            srcAddr = le(regs.srcAddr) & (channel == DMA0 ? 0x07FFFFFF : 0xFFFFFFF);
            destAddr = le(regs.destAddr) & (channel == DMA3 ? 0xFFFFFFF : 0x07FFFFFF);
            fetchCount();

            if (condition == SPECIAL) {
                std::cout << "ERROR: DMA" << std::dec << static_cast<uint32_t>(channel) << " timing: special not yet supported" << std::endl;

                //TODO init for special timing!
                if (channel == DMA1 || channel == DMA2) {
                    /*
                    Sound DMA (FIFO Timing Mode) (DMA1 and DMA2 only)
                    In this mode, the DMA Repeat bit must be set, and the destination address must be FIFO_A (040000A0h) or FIFO_B (040000A4h).
                    Upon DMA request from sound controller, 4 units of 32bits (16 bytes) are transferred (both Word Count register and DMA Transfer Type bit are ignored).
                    The destination address will not be incremented in FIFO mode.
                    */
                    count = 4;
                    width32Bit = true;
                    srcCnt = INCREMENT;
                    dstCnt = FIXED;
                } else if (channel == DMA3) {
                    /*
                    Video Capture Mode (DMA3 only)
                    Intended to copy a bitmap from memory (or from external hardware/camera) to VRAM. 
                    When using this transfer mode, set the repeat bit, and write the number of data units (per scanline) to the word count register. 
                    Capture works similar like HBlank DMA, however, the transfer is started when VCOUNT=2, 
                    it is then repeated each scanline, and it gets stopped when VCOUNT=162.
                    */
                }
            }
        }

        return enable;
    }

    void DMA::fetchCount()
    {
        count = le(regs.count) & (channel == DMA3 ? 0x0FFFF : 0x3FFF);
        // 0 is used for the max value possible
        count = count == 0 ? (channel == DMA3 ? 0x10000 : 0x4000) : count;
    }

    bool DMA::conditionSatisfied() const
    {
        uint16_t dispstat = memory.ioHandler.externalRead16(0x04000004);
        bool vBlank = dispstat & 1;
        bool hBlank = dispstat & 2;

        switch (condition) {
            case NO_COND:
                // Nothing to do here
                break;
            case WAIT_VBLANK:
                // Wait for VBLANK
                return vBlank;
            case WAIT_HBLANK:
                return hBlank;
            case SPECIAL:
                //TODO find out what to do
                // The 'Special' setting (Start Timing=3) depends on the DMA channel: DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
                return false;
        }

        return true;
    }

} // namespace gbaemu
