#ifndef DMA_HPP
#define DMA_HPP

#include "memory.hpp"
#include "packed.h"

#include <cstdint>
#include <functional>

namespace gbaemu
{

    // Forward declarations
    namespace lcd
    {
        class LCDController;
    }
    class CPU;
    class InterruptHandler;

    class DMAGroup
    {
      private:
        enum DMAChannel : uint8_t {
            DMA0 = 0,
            DMA1,
            DMA2,
            DMA3
        };

        enum DMAState {
            IDLE,
            STARTED,
            REPEAT,
            WAITING_PAUSED,
            SEQ_COPY,
            DONE
        };

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

        bool conditionSatisfied(StartCondition condition) const;
        static const char *countTypeToStr(AddrCntType updateKind);

        template <DMAChannel channel>
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

            static const constexpr uint16_t DMA_CNT_REG_EN_MASK = static_cast<uint16_t>(1) << DMA_CNT_REG_EN_OFF;
            static const constexpr uint16_t DMA_CNT_REG_IRQ_MASK = static_cast<uint16_t>(1) << DMA_CNT_REG_IRQ_OFF;
            static const constexpr uint16_t DMA_CNT_REG_DRQ_MASK = static_cast<uint16_t>(1) << DMA_CNT_REG_DRQ_OFF;
            static const constexpr uint16_t DMA_CNT_REG_TYPE_MASK = static_cast<uint16_t>(1) << DMA_CNT_REG_TYPE_OFF;
            static const constexpr uint16_t DMA_CNT_REG_REPEAT_MASK = static_cast<uint16_t>(1) << DMA_CNT_REG_REPEAT_OFF;
            static const constexpr uint16_t DMA_CNT_REG_TIMING_MASK = static_cast<uint16_t>(3) << DMA_CNT_REG_TIMING_OFF;
            static const constexpr uint16_t DMA_CNT_REG_SRC_ADR_CNT_MASK = static_cast<uint16_t>(3) << DMA_CNT_REG_SRC_ADR_CNT_OFF;
            static const constexpr uint16_t DMA_CNT_REG_DST_ADR_CNT_MASK = static_cast<uint16_t>(3) << DMA_CNT_REG_DST_ADR_CNT_OFF;

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

            /*
        DMA Transfer Channels
          40000B0h  4    W    DMA0SAD   DMA 0 Source Address
          40000B4h  4    W    DMA0DAD   DMA 0 Destination Address
          40000B8h  2    W    DMA0CNT_L DMA 0 Word Count
          40000BAh  2    R/W  DMA0CNT_H DMA 0 Control
          40000BCh  4    W    DMA1SAD   DMA 1 Source Address
          40000C0h  4    W    DMA1DAD   DMA 1 Destination Address
          40000C4h  2    W    DMA1CNT_L DMA 1 Word Count
          40000C6h  2    R/W  DMA1CNT_H DMA 1 Control
          40000C8h  4    W    DMA2SAD   DMA 2 Source Address
          40000CCh  4    W    DMA2DAD   DMA 2 Destination Address
          40000D0h  2    W    DMA2CNT_L DMA 2 Word Count
          40000D2h  2    R/W  DMA2CNT_H DMA 2 Control
          40000D4h  4    W    DMA3SAD   DMA 3 Source Address
          40000D8h  4    W    DMA3DAD   DMA 3 Destination Address
          40000DCh  2    W    DMA3CNT_L DMA 3 Word Count
          40000DEh  2    R/W  DMA3CNT_H DMA 3 Control
          40000E0h       -    -         Not used
        */

          private:
            DMAState state;
            Memory &memory;
            InterruptHandler &irqHandler;

            DMAGroup &dmaGroup;

            PACK_STRUCT(DMARegs, regs,
                        uint32_t srcAddr;
                        uint32_t destAddr;
                        uint16_t count;
                        uint16_t cntReg;);

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

            uint8_t read8FromReg(uint32_t offset);
            void write8ToReg(uint32_t offset, uint8_t value);

          public:
            DMA(CPU *cpu, DMAGroup &dmaGroup);

            void step(InstructionExecutionInfo &info, uint32_t cycles);

            void reset();

          private:
            void extractRegValues();
            void updateAddr(uint32_t &addr, AddrCntType updateKind) const;
            void fetchCount();
        };

        DMA<DMA0> dma0;
        DMA<DMA1> dma1;
        DMA<DMA2> dma2;
        DMA<DMA3> dma3;

        uint8_t dmaEnableBitset;

        const lcd::LCDController *lcdController;

        const std::function<void(InstructionExecutionInfo &, uint32_t)> stepLUT[16] = {
            // DMA3 DMA2 DMA1 DMA0
            // 0    0    0    0
            [](InstructionExecutionInfo &, uint32_t) {},
            // DMA3 DMA2 DMA1 DMA0
            // 0    0    0    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 0    0    1    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma1.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 0    0    1    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma1.step(info, cycles);
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 0    1    0    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma2.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 0    1    0    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma2.step(info, cycles);
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 0    1    1    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma2.step(info, cycles);
                this->dma1.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 0    1    1    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma2.step(info, cycles);
                this->dma1.step(info, cycles);
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    0    0    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    0    0    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    0    1    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma1.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    0    1    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma1.step(info, cycles);
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    1    0    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma2.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    1    0    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma2.step(info, cycles);
                this->dma0.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    1    1    0
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma2.step(info, cycles);
                this->dma1.step(info, cycles);
            },
            // DMA3 DMA2 DMA1 DMA0
            // 1    1    1    1
            [this](InstructionExecutionInfo &info, uint32_t cycles) {
                this->dma3.step(info, cycles);
                this->dma2.step(info, cycles);
                this->dma1.step(info, cycles);
                this->dma0.step(info, cycles);
            }};

      public:
        DMAGroup(CPU *cpu) : dma0(cpu, *this), dma1(cpu, *this), dma2(cpu, *this), dma3(cpu, *this)
        {
          reset();
        }

        void setLCDController(const lcd::LCDController *lcdController)
        {
            this->lcdController = lcdController;
        }

        void step(InstructionExecutionInfo &info, uint32_t cycles)
        {
            stepLUT[dmaEnableBitset](info, cycles);
        }

        void reset()
        {
            dma0.reset();
            dma1.reset();
            dma2.reset();
            dma3.reset();
            dmaEnableBitset = 0;
        }
    };

} // namespace gbaemu
#endif