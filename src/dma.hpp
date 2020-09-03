#ifndef DMA_HPP
#define DMA_HPP

#include "memory.hpp"
#include <cstdint>

namespace gbaemu
{

    class DMA
    {
      private:
        static const constexpr uint8_t DMA_CNT_REG_TIMING_OFF = 12;
        static const constexpr uint8_t DMA_CNT_REG_SRC_ADR_CNT_OFF = 7;
        static const constexpr uint8_t DMA_CNT_REG_DST_ADR_CNT_OFF = 5;
        static const constexpr uint8_t DMA_CNT_REG_EN_OFF = 15;
        static const constexpr uint8_t DMA_CNT_REG_IRQ_OFF = 14;
        static const constexpr uint8_t DMA_CNT_REG_DRQ_OFF = 11;
        static const constexpr uint8_t DMA_CNT_REG_TYPE_OFF = 10;
        static const constexpr uint8_t DMA_CNT_REG_REPEAT_OFF = 9;

        static const constexpr uint32_t DMA_CNT_REG_EN_MASK = static_cast<uint32_t>(1) << DMA_CNT_REG_EN_OFF;
        static const constexpr uint32_t DMA_CNT_REG_IRQ_MASK = static_cast<uint32_t>(1) << DMA_CNT_REG_IRQ_OFF;
        static const constexpr uint32_t DMA_CNT_REG_DRQ_MASK = static_cast<uint32_t>(1) << DMA_CNT_REG_DRQ_OFF;
        static const constexpr uint32_t DMA_CNT_REG_TYPE_MASK = static_cast<uint32_t>(1) << DMA_CNT_REG_TYPE_OFF;
        static const constexpr uint32_t DMA_CNT_REG_REPEAT_MASK = static_cast<uint32_t>(1) << DMA_CNT_REG_REPEAT_OFF;
        static const constexpr uint32_t DMA_CNT_REG_TIMING_MASK = static_cast<uint32_t>(3) << DMA_CNT_REG_TIMING_OFF;
        static const constexpr uint32_t DMA_CNT_REG_SRC_ADR_CNT_MASK = static_cast<uint32_t>(3) << DMA_CNT_REG_SRC_ADR_CNT_OFF;
        static const constexpr uint32_t DMA_CNT_REG_DST_ADR_CNT_MASK = static_cast<uint32_t>(3) << DMA_CNT_REG_DST_ADR_CNT_OFF;

      public:
        static const constexpr uint32_t DMA0_BASE_ADDR = Memory::IO_REGS_OFFSET | 0x0B0;
        static const constexpr uint32_t DMA1_BASE_ADDR = Memory::IO_REGS_OFFSET | 0x0BC;
        static const constexpr uint32_t DMA2_BASE_ADDR = Memory::IO_REGS_OFFSET | 0x0C8;
        static const constexpr uint32_t DMA3_BASE_ADDR = Memory::IO_REGS_OFFSET | 0x0D4;
        static const constexpr uint32_t DMA_BASE_ADDRESSES[] = {
            DMA0_BASE_ADDR,
            DMA1_BASE_ADDR,
            DMA2_BASE_ADDR,
            DMA3_BASE_ADDR,
        };

        enum DMAState {
            IDLE,
            STARTED,
            REPEAT,
            WAITING_PAUSED,
            SEQ_COPY,
            DONE
        };

        enum DMAChannel : uint8_t {
            DMA0 = 0,
            DMA1,
            DMA2,
            DMA3
        };

        struct DMARegs {
            uint32_t srcAddr;
            uint32_t destAddr;
            uint32_t count;
            uint16_t cntReg;
        } __attribute__((packed));

      private:
        const DMAChannel channel;
        DMAState state;
        Memory &memory;
        DMARegs regs = {0};

        enum AddrCntType : uint8_t {
            INCREMENT = 0,
            DECREMENT = 1,
            FIXED = 2,
            INCREMENT_RELOAD = 3
        };

        enum StartCondition : uint8_t {
            NO_COND = 0,
            WAIT_VBLANK = 1,
            WAIT_HBLANK = 2, // When accessing OAM (7000000h) or OBJ VRAM (6010000h) by HBlank Timing, then the "H-Blank Interval Free" bit in DISPCNT register must be set
            SPECIAL = 3      // The 'Special' setting (Start Timing=3) depends on the DMA channel: DMA0=Prohibited, DMA1/DMA2=Sound FIFO, DMA3=Video Capture
        };

        // Extracted control reg values:
        uint32_t srcAddr;
        uint32_t destAddr;
        uint32_t count;
        bool repeat;
        //TODO no idea what this is
        bool gamePakDRQ;
        bool irqOnEnd;
        bool width32Bit;
        AddrCntType srcCnt;
        AddrCntType dstCnt;
        StartCondition condition;

        uint8_t read8FromReg(uint32_t addr)
        {
            //TODO endianess???
            return *((addr - DMA_BASE_ADDRESSES[channel]) + reinterpret_cast<uint8_t *>(&regs));
        }
        void write8ToReg(uint32_t addr, uint8_t value)
        {
            //TODO endianess???
            *((addr - DMA_BASE_ADDRESSES[channel]) + reinterpret_cast<uint8_t *>(&regs)) = value;
        }

      public:
        DMA(DMAChannel channel, Memory &memory) : channel(channel), state(IDLE), memory(memory)
        {
            memory.ioHandler.registerIOMappedDevice(
                IO_Mapped(
                    DMA_BASE_ADDRESSES[channel],
                    DMA_BASE_ADDRESSES[channel] + sizeof(regs),
                    std::bind(&DMA::read8FromReg, this, std::placeholders::_1),
                    std::bind(&DMA::write8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&DMA::read8FromReg, this, std::placeholders::_1),
                    std::bind(&DMA::write8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
        }

        InstructionExecutionInfo step();

      private:
        bool extractRegValues();
        void updateAddr(uint32_t &addr, AddrCntType updateKind) const;
        bool conditionSatisfied() const;
        void fetchCount();
        bool checkForUserAbort();
    };

} // namespace gbaemu
#endif