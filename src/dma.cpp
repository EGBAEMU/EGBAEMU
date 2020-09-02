#include "dma.hpp"
#include "util.hpp"

namespace gbaemu
{

    InstructionExecutionInfo DMA::step(bool execute)
    {
        InstructionExecutionInfo info{0};

        // FSM actions
        switch (state) {
            case IDLE:
                if (extractRegValues()) {
                    state = conditionSatisfied() ? STARTED : WAITING_PAUSED;
                    std::cout << "INFO: Registered DMA" << channel << " transfer request." << std::endl;
                    std::cout << "      Source Addr: 0x" << std::hex << srcAddr << std::endl;
                    std::cout << "      Dest Addr:   0x" << std::hex << destAddr << std::endl;
                    std::cout << "      Words: 0x" << std::hex << count << std::endl;
                }
                break;

            case WAITING_PAUSED: {
                //TODO check for abort
                state = conditionSatisfied() ? STARTED : WAITING_PAUSED;
                break;
            }

            case STARTED: {
                //TODO check for abort!
                state = SEQ_COPY;

                if (width32Bit) {
                    uint32_t data = memory.read32(srcAddr, &info, false);
                    memory.write16(destAddr, data, &info, false);
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
                //TODO check for abort!

                if (count == 0) {
                    state = DONE;
                } else {
                    if (width32Bit) {
                        uint32_t data = memory.read32(srcAddr, &info, true);
                        memory.write16(destAddr, data, &info, true);
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
                    //TODO check for abort by user, but how does he signal an abort??? Can we just go back to IDLE and parse everything again? (and dont clear the enable bit)
                    state = STARTED;
                } else {
                    // return to idle state
                    state = IDLE;

                    // Clear enable bit to indicate that we are done!
                    regs->cntReg &= ~DMA_CNT_REG_EN_MASK;

                    if (irqOnEnd) {
                        //TODO trigger interrupt!!!
                        std::cout << "ERROR: DMA trigger interrupt not yet supported" << std::endl;
                    }
                }

                break;
            }
        }

        return info;
    }

    void DMA::updateAddr(uint32_t &addr, AddrCntType updateKind) const
    {
        switch (updateKind) {
            case INCREMENT:
                addr += width32Bit ? 4 : 2;
                break;
            case DECREMENT:
                addr -= width32Bit ? 4 : 2;
                break;
            case FIXED:
                // Nothing to do here
                break;
            case INCREMENT_RELOAD:
                //TODO what are you???
                std::cout << "ERROR: DMA increment_reload: not yet supported" << std::endl;
                break;
        }
    }

    bool DMA::extractRegValues()
    {
        const uint16_t controlReg = le(regs->cntReg);
        bool enable = controlReg & DMA_CNT_REG_EN_MASK;

        if (enable) {
            // Extract remaining values
            repeat = controlReg & DMA_CNT_REG_REPEAT_MASK;
            gamePakDRQ = controlReg & DMA_CNT_REG_DRQ_MASK;
            irqOnEnd = controlReg & DMA_CNT_REG_IRQ_MASK;
            width32Bit = controlReg & DMA_CNT_REG_TYPE_MASK;
            srcCnt = static_cast<AddrCntType>((controlReg & DMA_CNT_REG_SRC_ADR_CNT_MASK) >> DMA_CNT_REG_SRC_ADR_CNT_OFF);
            dstCnt = static_cast<AddrCntType>((controlReg & DMA_CNT_REG_DST_ADR_CNT_MASK) >> DMA_CNT_REG_DST_ADR_CNT_OFF);
            condition = static_cast<StartCondition>((controlReg & DMA_CNT_REG_TIMING_MASK >> DMA_CNT_REG_TIMING_OFF));

            // Mask out ignored bits
            srcAddr = le(regs->srcAddr) & (channel == DMA0 ? 0x07FFFFFF : 0xFFFFFFF);
            destAddr = le(regs->destAddr) & (channel == DMA3 ? 0xFFFFFFF : 0x07FFFFFF);
            count = le(regs->count) & (channel == DMA3 ? 0x0FFFF : 0x3FFF);
            // 0 is used for the max value possible
            count = count == 0 ? (channel == DMA3 ? 0x10000 : 0x4000) : count;
        }

        return enable;
    }

    bool DMA::conditionSatisfied() const
    {
        switch (condition) {
            case NO_COND:
                // Nothing to do here
                break;
            case WAIT_VBLANK:
                //TODO find out what to do
                std::cout << "ERROR: DMA timing: vblank not yet supported" << std::endl;
                break;
            case WAIT_HBLANK:
                //TODO find out what to do
                std::cout << "ERROR: DMA timing: hblank not yet supported" << std::endl;
                break;
            case SPECIAL:
                //TODO find out what to do
                // The 'Special' setting (Start Timing=3) depends on the DMA channel: DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
                std::cout << "ERROR: DMA timing: special not yet supported" << std::endl;
                break;
        }
        return true;
    }

} // namespace gbaemu
