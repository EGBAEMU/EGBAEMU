
#include "dma.hpp"
#include "cpu/cpu.hpp"
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

    template <DMAGroup::DMAChannel channel>
    uint8_t DMAGroup::DMA<channel>::read8FromReg(uint32_t offset)
    {
        return *(offset + reinterpret_cast<uint8_t *>(&regs));
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::write8ToReg(uint32_t offset, uint8_t value)
    {
        *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;

        if (offset == offsetof(DMARegs, cntReg) + sizeof(regs.cntReg) - 1) {
            // Update the enable bitset to signal if dma is enabled or not!
            dmaGroup.dmaEnableBitset = bitSet<uint8_t, 1, channel>(dmaGroup.dmaEnableBitset, bmap<uint8_t>(isBitSet<uint8_t, DMA_CNT_REG_EN_OFF - (sizeof(regs.cntReg) - 1) * 8>(value)));
            state = IDLE;
        }
    }

    template <DMAGroup::DMAChannel channel>
    DMAGroup::DMA<channel>::DMA(CPU *cpu, DMAGroup &dmaGroup) : memory(cpu->state.memory), irqHandler(cpu->irqHandler), dmaGroup(dmaGroup)
    {
        memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                DMA_BASE_ADDRESSES[channel],
                DMA_BASE_ADDRESSES[channel] + sizeof(regs) - 1,
                std::bind(&DMAGroup::DMA<channel>::read8FromReg, this, std::placeholders::_1),
                std::bind(&DMAGroup::DMA<channel>::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&DMAGroup::DMA<channel>::read8FromReg, this, std::placeholders::_1),
                std::bind(&DMAGroup::DMA<channel>::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
    }

    template <DMAGroup::DMAChannel channel>
    void DMAGroup::DMA<channel>::step(InstructionExecutionInfo &info, uint32_t cycles)
    {
        // Check if enabled only once, because there can not be state changes, after dma has taken over control!
        // We can safely assume that this function does not get called if dma is disabled!

        do {
            // FSM actions
            switch (state) {
                case IDLE:
                    extractRegValues();
                    state = dmaGroup.conditionSatisfied(condition) ? STARTED : WAITING_PAUSED;
                    LOG_DMA(
                        std::cout << "INFO: Registered DMA" << std::dec << static_cast<uint32_t>(channel) << " transfer request." << std::endl;
                        std::cout << "      Source Addr: 0x" << std::hex << srcAddr << " Type: " << DMAGroup::countTypeToStr(srcCnt) << std::endl;
                        std::cout << "      Dest Addr:   0x" << std::hex << destAddr << " Type: " << DMAGroup::countTypeToStr(dstCnt) << std::endl;
                        std::cout << "      Words: 0x" << std::hex << count << std::endl;
                        std::cout << "      Repeat: " << repeat << std::endl;
                        std::cout << "      GamePak DRQ: " << gamePakDRQ << std::endl;
                        std::cout << "      32 bit mode: " << width32Bit << std::endl;
                        std::cout << "      IRQ on end: " << irqOnEnd << std::endl;);

                    // If eeprom does not yet know for sure which bus width it has we can determine it by passing DMA request to it!
                    if (channel == DMA3 && memory.getBackupType() == Memory::EEPROM_V && !memory.eeprom->knowsBitWidth()) {

                        // We only need changes for 14 Bit buswidth as we default to 8 bit
                        memory.normalizeAddressRef(srcAddr, info);
                        if (info.memReg == Memory::MemoryRegion::EEPROM_REGION) {
                            const constexpr uint32_t bus14BitReadExpectedCount = 17;
                            const constexpr uint32_t bus6BitReadExpectedCount = 9;
                            if (count == bus14BitReadExpectedCount) {
                                memory.eeprom->expand(14);
                                break;
                            } else if (count == bus6BitReadExpectedCount) {
                                memory.eeprom->expand(6);
                                break;
                            }
                        }

                        memory.normalizeAddressRef(destAddr, info);
                        if (info.memReg == Memory::MemoryRegion::EEPROM_REGION) {
                            const constexpr uint32_t bus14BitWriteExpectedCount = 81;
                            const constexpr uint32_t bus6BitWriteExpectedCount = 73;
                            if (count == bus14BitWriteExpectedCount) {
                                memory.eeprom->expand(14);
                            } else if (count == bus6BitWriteExpectedCount) {
                                memory.eeprom->expand(6);
                            }
                        }
                    }

                    break;

                case REPEAT: {

                    //TODO reload only after condition is satisfied???
                    if (dstCnt == INCREMENT_RELOAD) {
                        destAddr = le(regs.destAddr) & (channel == DMA3 ? 0xFFFFFFF : 0x07FFFFFF);
                        //TODO is the src address kept?
                        // srcAddr = le(regs.srcAddr) & (channel == DMA0 ? 0x07FFFFFF : 0xFFFFFFF);
                    }

                    fetchCount();

                    state = WAITING_PAUSED;
                    // Fall through to check the condition
                }

                case WAITING_PAUSED: {
                    state = dmaGroup.conditionSatisfied(condition) ? STARTED : WAITING_PAUSED;
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
                    if (repeat) {
                        state = REPEAT;
                    } else {
                        // return to idle state
                        state = IDLE;

                        // Clear enable bit to indicate that we are done!
                        regs.cntReg &= ~le(DMA_CNT_REG_EN_MASK);
                        dmaGroup.dmaEnableBitset &= ~(1 << channel);

                        if (irqOnEnd) {
                            irqHandler.setInterrupt(static_cast<InterruptHandler::InterruptType>(InterruptHandler::InterruptType::DMA_0 + channel));
                        }
                    }

                    break;
                }
            }
        } while (state != IDLE && state != WAITING_PAUSED && info.cycleCount < cycles);
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
    void DMAGroup::DMA<channel>::extractRegValues()
    {
        const uint16_t controlReg = le(regs.cntReg);

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
        switch (condition) {
            case NO_COND:
                // Nothing to do here
                break;
            case WAIT_VBLANK:
                // Wait for VBLANK
                return lcdController->isVBlank();
            case WAIT_HBLANK:
                return lcdController->isHBlank();
            case SPECIAL:
                //TODO find out what to do
                // The 'Special' setting (Start Timing=3) depends on the DMA channel: DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
                return false;
        }

        return true;
    }

    template class DMAGroup::DMA<DMAGroup::DMA0>;
    template class DMAGroup::DMA<DMAGroup::DMA1>;
    template class DMAGroup::DMA<DMAGroup::DMA2>;
    template class DMAGroup::DMA<DMAGroup::DMA3>;

} // namespace gbaemu
