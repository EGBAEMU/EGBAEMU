#include "interrupts.hpp"
#include "cpu/cpu.hpp"
#include "cpu/regs.hpp"
#include "logging.hpp"
#include "memory.hpp"
#include "util.hpp"

#include <algorithm>
#include <functional>
#include <iostream>

namespace gbaemu
{
    const uint32_t InterruptHandler::INTERRUPT_CONTROL_REG_ADDR = Memory::IO_REGS_OFFSET + 0x200;

    uint8_t InterruptHandler::read8FromReg(uint32_t offset) const
    {
        return *(offset + reinterpret_cast<const uint8_t *>(&regs));
    }

    void InterruptHandler::internalWrite8ToReg(uint32_t offset, uint8_t value)
    {
        if (offset == offsetof(InterruptControlRegs, irqRequest) || offset == offsetof(InterruptControlRegs, irqRequest) + 1) {
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        } else {
            LOG_IRQ(std::cout << "WARNING: internal write to non IRQ request registers!" << std::endl;);
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        }
    }

    void InterruptHandler::externalWrite8ToReg(uint32_t offset, uint8_t value)
    {
        if (offset == offsetof(InterruptControlRegs, irqRequest) || offset == offsetof(InterruptControlRegs, irqRequest) + 1) {
            *(offset + reinterpret_cast<uint8_t *>(&regs)) &= ~value;
        } else if (offset == offsetof(InterruptControlRegs, waitStateCnt) || offset == offsetof(InterruptControlRegs, waitStateCnt) + 1) {
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
            cpu->state.memory.updateWaitCycles(le(regs.waitStateCnt));
        } else {
            if (offset == offsetof(InterruptControlRegs, irqMasterEnable)) {
                // We store in LSB so this is fine!
                masterIRQEn = value & 0x1;
            }

            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        }
    }

    void InterruptHandler::reset()
    {
        std::fill_n(reinterpret_cast<char *>(&regs), sizeof(regs), 0);
    }

    InterruptHandler::InterruptHandler(CPU *cpu) : cpu(cpu)
    {
        reset();
        cpu->state.memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                INTERRUPT_CONTROL_REG_ADDR,
                INTERRUPT_CONTROL_REG_ADDR + sizeof(regs) - 1,
                std::bind(&InterruptHandler::read8FromReg, this, std::placeholders::_1),
                std::bind(&InterruptHandler::externalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&InterruptHandler::read8FromReg, this, std::placeholders::_1),
                std::bind(&InterruptHandler::internalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2)));

        masterIRQEn = false;
    }

    template <InterruptHandler::InterruptType type>
    void InterruptHandler::setInterrupt()
    {
        regs.irqRequest |= le(static_cast<uint16_t>(static_cast<uint16_t>(1) << type));
    }

    void InterruptHandler::checkForInterrupt()
    {
        bool triggerIRQ = regs.irqRequest & regs.irqEnable & le<uint16_t>(0x3FFF);

        // We can only execute the interrupt if all conditions are met
        if (masterIRQEn && !cpu->state.getFlag<cpsr_flags::IRQ_DISABLE>() && triggerIRQ) {
            /*
            BIOS Interrupt handling
            Upon interrupt execution, the CPU is switched into IRQ mode, and the physical interrupt vector is called - as this address
             is located in BIOS ROM, the BIOS will always execute the following code before it forwards control to the user handler:
            1. switch to arm mode
            2. save CPSR into SPSR_irq
            3. save PC (or next PC) into LR_irq
            4. switch to IRQ mode (also set CPSR accordingly)
            5. jump to ROM appended bios irq handler
            (other orders might be better)
            00000124  b pc -8                    ; infinite loop as protection barrior between ROM code & bios code
            00000128  stmfd  r13!,r0-r3,r12,r14  ;save registers to SP_irq
            0000012C  mov    r0,4000000h         ;ptr+4 to 03FFFFFC (mirror of 03007FFC)
            00000130  add    r14,r15,0h          ;retadr for USER handler $+8=138h
            00000134  ldr    r15,[r0,-4h]        ;jump to [03FFFFFC] USER handler
            00000138  ldmfd  r13!,r0-r3,r12,r14  ;restore registers from SP_irq
            0000013C  subs   r15,r14,4h          ;return from IRQ (PC=LR-4, CPSR=SPSR)
            As shown above, a pointer to the 32bit/ARM-code user handler must be setup in [03007FFCh]. By default, 160 bytes of memory 
            are reserved for interrupt stack at 03007F00h-03007F9Fh.
            */

            // Save the current CPSR register value into SPSR_irq
            *(cpu->state.getModeRegs(CPUState::IRQ)[regs::SPSR_OFFSET]) = cpu->state.accessReg(regs::CPSR_OFFSET);
            // Save PC to LR_irq
            *(cpu->state.getModeRegs(CPUState::IRQ)[regs::LR_OFFSET]) = cpu->state.getCurrentPC() + 4;

            // Change instruction mode to arm
            cpu->decodeAndExecute = cpu->armDecodeAndExecutor;

            // Change the register mode to irq
            // Ensure that the CPSR represents that we are in ARM mode again
            // Clear all flags & enforce irq mode
            // Also disable interrupts
            cpu->state.accessReg(regs::CPSR_OFFSET) = 0b010010 | (1 << 7);
            cpu->state.accessReg(regs::PC_OFFSET) = Memory::BIOS_IRQ_HANDLER_OFFSET;

            cpu->state.updateCPUMode();

            // Flush the pipeline
            cpu->state.normalizePC(false);
            cpu->initPipeline();
        }
    }

    bool InterruptHandler::checkForHaltCondition(uint32_t mask)
    {
        /*
            Halt mode is terminated when any enabled interrupts are requested, 
            that is when (IE AND IF) is not zero, the GBA locks up if that condition doesn't get true.
        */
        mask &= 0x3FFF;
        uint16_t irqRequestReg = le(regs.irqRequest) & 0x3FFF;
        return irqRequestReg & mask;
    }

    template void InterruptHandler::setInterrupt<InterruptHandler::LCD_V_BLANK>();
    template void InterruptHandler::setInterrupt<InterruptHandler::LCD_H_BLANK>();
    template void InterruptHandler::setInterrupt<InterruptHandler::LCD_V_COUNTER_MATCH>();
    template void InterruptHandler::setInterrupt<InterruptHandler::TIMER_0_OVERFLOW>();
    template void InterruptHandler::setInterrupt<InterruptHandler::TIMER_1_OVERFLOW>();
    template void InterruptHandler::setInterrupt<InterruptHandler::TIMER_2_OVERFLOW>();
    template void InterruptHandler::setInterrupt<InterruptHandler::TIMER_3_OVERFLOW>();
    // template void InterruptHandler::setInterrupt<InterruptHandler::SERIAL_COMM>();
    template void InterruptHandler::setInterrupt<InterruptHandler::DMA_0>();
    template void InterruptHandler::setInterrupt<InterruptHandler::DMA_1>();
    template void InterruptHandler::setInterrupt<InterruptHandler::DMA_2>();
    template void InterruptHandler::setInterrupt<InterruptHandler::DMA_3>();
    template void InterruptHandler::setInterrupt<InterruptHandler::KEYPAD>();
    // template void InterruptHandler::setInterrupt<InterruptHandler::GAME_PAK>();
} // namespace gbaemu