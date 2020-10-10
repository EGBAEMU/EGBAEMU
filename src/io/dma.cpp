
#include "dma.hpp"
#include "cpu/cpu.hpp"
#include "decode/inst.hpp"
#include "interrupts.hpp"
#include "lcd/lcd-controller.hpp"
#include "logging.hpp"
#include "memory.hpp"
#include "util.hpp"

#include <algorithm>
#include <functional>
#include <iostream>

namespace gbaemu
{
    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::reset()
    {
        state = IDLE;
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
    }

    const char *DMAGroup::countTypeToStr(AddrCntType updateKind)
    {
        switch (updateKind) {
            STRINGIFY_CASE_ID(INCREMENT_RELOAD);
            STRINGIFY_CASE_ID(INCREMENT);
            STRINGIFY_CASE_ID(DECREMENT);
            STRINGIFY_CASE_ID(FIXED);
        }
        return "";
    }

    const char *DMAGroup::startCondToStr(StartCondition condition)
    {
        switch (condition) {
            STRINGIFY_CASE_ID(NO_COND);
            STRINGIFY_CASE_ID(WAIT_HBLANK);
            STRINGIFY_CASE_ID(WAIT_VBLANK);
            STRINGIFY_CASE_ID(SPECIAL);
        }
        return "";
    }

    template <DMAGroup::DMAChannel channel>
    uint8_t DMAGroup::DMA<channel>::read8FromReg(uint32_t offset)
    {
        return *(offset + reinterpret_cast<uint8_t *>(&regs));
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::write8ToReg(uint32_t offset, uint8_t value)
    {
        if (offset == offsetof(DMARegs, cntReg) + sizeof(regs.cntReg) - 1) {
            // Update the enable bitset to signal if dma is enabled or not!
            dmaGroup.dmaEnableBitset = bitSet<uint8_t, 1, channel>(dmaGroup.dmaEnableBitset, bmap<uint8_t>(isBitSet<uint8_t, DMA_CNT_REG_EN_OFF - (sizeof(regs.cntReg) - 1) * 8>(value)));
            state = IDLE;

            // game pak is only for DMA3
            value &= (channel == DMA3 ? 0xFF : 0xF7);
        } else if (offset == offsetof(DMARegs, cntReg)) {
            // Mask out unused bits
            value &= 0xE0;
        }

        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
    }

    template <DMAGroup::DMAChannel channel>
    DMAGroup::DMA<channel>::DMA(CPU *cpu, DMAGroup &dmaGroup) : memory(cpu->state.memory), irqHandler(cpu->irqHandler), dmaGroup(dmaGroup)
    {
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::step(InstructionExecutionInfo &info, uint32_t cycles)
    {
        // Check if enabled only once, because there can not be state changes, after dma has taken over control!
        // We can safely assume that this function does not get called if dma is disabled!

        // Ensure that we are on budget
        if (info.cycleCount < cycles)
            do {
                // FSM actions
                switch (state) {
                    case IDLE:
                        extractConfig();
                        // If the DMA was enabled we need to reload the values in ANY case even if repeat is set
                        if (dmaGroup.conditionSatisfied(condition)) {
                            state = LOAD_VALUES;
                        } else {
                            goToWaitingState();
                        }
                        break;

                    case REPEAT: {
                        // reload the values after the condition was satisfied!
                        if (dstCnt == INCREMENT_RELOAD) {
                            destAddr = le(regs.destAddr) & (channel == DMA3 ? 0xFFFFFFF : 0x07FFFFFF);
                            //TODO is the src address kept?
                            //srcAddr = le(regs.srcAddr) & (channel == DMA0 ? 0x07FFFFFF : 0xFFFFFFF);
                        }

                        fetchCount();

                        state = STARTED;
                        break;
                    }

                    case LOAD_VALUES: {
                        // Load all runtime values: destAddr, srcAddr, count
                        reloadValues();

                        LOG_DMA(
                            std::cout << "INFO: Registered DMA" << std::dec << static_cast<uint32_t>(channel) << " transfer request." << std::endl;
                            std::cout << "      Source Addr: 0x" << std::hex << srcAddr << " Type: " << DMAGroup::countTypeToStr(srcCnt) << std::endl;
                            std::cout << "      Dest Addr:   0x" << std::hex << destAddr << " Type: " << DMAGroup::countTypeToStr(dstCnt) << std::endl;
                            std::cout << "      Timing: " << DMAGroup::startCondToStr(condition) << std::endl;
                            std::cout << "      Words: 0x" << std::hex << count << std::endl;
                            std::cout << "      Repeat: " << repeat << std::endl;
                            if (channel == DMA3) {
                                std::cout << "      GamePak DRQ: " << gamePakDRQ << std::endl;
                            } std::cout
                            << "      32 bit mode: " << width32Bit << std::endl;
                            std::cout << "      IRQ on end: " << irqOnEnd << std::endl;);

                        // Initialization takes at least 2 cycles
                        info.cycleCount += 2;

                        // If eeprom does not yet know for sure which bus width it has we can determine it by passing DMA request to it!
                        if (channel == DMA3 && memory.rom.eepromNeedsInit()) {
                            memory.rom.initEEPROM(srcAddr, destAddr, count);
                        }

                        state = STARTED;
                        break;
                    }

                    case STARTED: {
                        state = SEQ_COPY;

                        if (width32Bit) {
                            uint32_t data = memory.read32(srcAddr, info, false, false, true);
                            memory.write32(destAddr, data, info, false);
                        } else {
                            uint16_t data = memory.read16(srcAddr, info, false, false, true);
                            memory.write16(destAddr, data, info, false);
                        }

                        --count;

                        updateAddr(srcAddr, srcCnt);
                        updateAddr(destAddr, dstCnt);

                        break;
                    }

                    case SEQ_COPY: {
                        if (count == 0) {
                            state = DONE;
                        } else {
                            if (width32Bit) {
                                uint32_t data = memory.read32(srcAddr, info, true, false, true);
                                memory.write32(destAddr, data, info, true);
                            } else {
                                uint16_t data = memory.read16(srcAddr, info, true, false, true);
                                memory.write16(destAddr, data, info, true);
                            }

                            --count;

                            updateAddr(srcAddr, srcCnt);
                            updateAddr(destAddr, dstCnt);
                        }
                        break;
                    }

                    case DONE: {
                        // Handle edge case: repeat enabled but no start condition -> infinite DMA transfers
                        if (repeat && condition != NO_COND) {
                            state = REPEAT_WAIT;
                        } else {
                            // return to idle state
                            state = IDLE;

                            // Clear enable bit to indicate that we are done!
                            regs.cntReg &= ~le(DMA_CNT_REG_EN_MASK);
                            dmaGroup.dmaEnableBitset &= ~(1 << channel);

                            if (irqOnEnd) {
                                irqHandler.setInterrupt<static_cast<InterruptHandler::InterruptType>(InterruptHandler::InterruptType::DMA_0 + channel)>();
                            }
                        }

                        break;
                    }

                    case REPEAT_WAIT:
                    case WAITING_PAUSED: {
                        // Should not occur!
                        break;
                    }
                }
            } while (state != IDLE && state != WAITING_PAUSED && state != REPEAT_WAIT && info.cycleCount < cycles);
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::goToWaitingState()
    {
        // indicate waiting, clear enable bit
        dmaGroup.dmaEnableBitset &= ~(1 << channel);
        state = WAITING_PAUSED;
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::updateAddr(uint32_t &addr, AddrCntType updateKind) const
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

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::extractConfig()
    {
        const uint16_t controlReg = le(regs.cntReg);

        // Extract remaining values
        repeat = controlReg & DMA_CNT_REG_REPEAT_MASK;
        if (channel == DMA3) {
            gamePakDRQ = controlReg & DMA_CNT_REG_DRQ_MASK;
        }
        irqOnEnd = controlReg & DMA_CNT_REG_IRQ_MASK;
        width32Bit = controlReg & DMA_CNT_REG_TYPE_MASK;
        srcCnt = static_cast<AddrCntType>((controlReg & DMA_CNT_REG_SRC_ADR_CNT_MASK) >> DMA_CNT_REG_SRC_ADR_CNT_OFF);
        dstCnt = static_cast<AddrCntType>((controlReg & DMA_CNT_REG_DST_ADR_CNT_MASK) >> DMA_CNT_REG_DST_ADR_CNT_OFF);
        condition = static_cast<StartCondition>((controlReg & DMA_CNT_REG_TIMING_MASK) >> DMA_CNT_REG_TIMING_OFF);
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::reloadValues()
    {
        // Mask out ignored bits
        srcAddr = le(regs.srcAddr) & (channel == DMA0 ? 0x07FFFFFF : 0xFFFFFFF);
        destAddr = le(regs.destAddr) & (channel == DMA3 ? 0xFFFFFFF : 0x07FFFFFF);
        fetchCount();

        if (condition == SPECIAL) {
            LOG_DMA(std::cout << "ERROR: DMA" << std::dec << static_cast<uint32_t>(channel) << " timing: special not yet supported" << std::endl;);

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

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::fetchCount()
    {
        count = le(regs.count) & (channel == DMA3 ? 0x0FFFF : 0x3FFF);
        // 0 is used for the max value possible
        count = count == 0 ? (channel == DMA3 ? 0x10000 : 0x4000) : count;
    }

    bool DMAGroup::conditionSatisfied(StartCondition condition) const
    {
        bool conditionSatisfied = true;
        switch (condition) {
            case NO_COND:
                // Nothing to do here
                break;
            case WAIT_VBLANK:
                // Wait for VBLANK
                conditionSatisfied = lcdController->isVBlank();
                break;
            case WAIT_HBLANK:
                conditionSatisfied = lcdController->isHBlank() && !lcdController->isVBlank();
                break;
            case SPECIAL:
                //TODO find out what to do
                // The 'Special' setting (Start Timing=3) depends on the DMA channel: DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
                conditionSatisfied = false;
                break;
        }

        return conditionSatisfied;
    }

    void DMAGroup::triggerCondition(StartCondition condition)
    {
        if ((dma0.state == WAITING_PAUSED || dma0.state == REPEAT_WAIT) && condition == dma0.condition) {
            dma0.state = static_cast<DMAState>(dma0.state + 1);
            dmaEnableBitset |= 1 << DMA0;
        }
        if ((dma1.state == WAITING_PAUSED || dma1.state == REPEAT_WAIT) && condition == dma1.condition) {
            dma1.state = static_cast<DMAState>(dma1.state + 1);
            dmaEnableBitset |= 1 << DMA1;
        }
        if ((dma2.state == WAITING_PAUSED || dma2.state == REPEAT_WAIT) && condition == dma2.condition) {
            dma2.state = static_cast<DMAState>(dma2.state + 1);
            dmaEnableBitset |= 1 << DMA2;
        }
        if ((dma3.state == WAITING_PAUSED || dma3.state == REPEAT_WAIT) && condition == dma3.condition) {
            dma3.state = static_cast<DMAState>(dma3.state + 1);
            dmaEnableBitset |= 1 << DMA3;
        }
    }

    template class DMAGroup::DMA<DMAGroup::DMA0>;
    template class DMAGroup::DMA<DMAGroup::DMA1>;
    template class DMAGroup::DMA<DMAGroup::DMA2>;
    template class DMAGroup::DMA<DMAGroup::DMA3>;

} // namespace gbaemu
