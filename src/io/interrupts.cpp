#include "interrupts.hpp"
#include "cpu/cpu.hpp"
#include "cpu/regs.hpp"
#include "logging.hpp"
#include "memory.hpp"
#include "util.hpp"

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
        if (offset == 2 || offset == 3) {
            regsMutex.lock();
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
            regsMutex.unlock();
        } else {
            LOG_IRQ(std::cout << "WARNING: internal write to non IRQ request registers!" << std::endl;);
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        }
    }

    void InterruptHandler::externalWrite8ToReg(uint32_t offset, uint8_t value)
    {
        if (offset == 2 || offset == 3) {
            regsMutex.lock();
            *(offset + reinterpret_cast<uint8_t *>(&regs)) &= ~value;
            regsMutex.unlock();
        } else {
            *(offset + reinterpret_cast<uint8_t *>(&regs)) = value;
        }
    }

    InterruptHandler::InterruptHandler(CPU *cpu) : cpu(cpu)
    {
        cpu->state.memory.ioHandler.registerIOMappedDevice(
            IO_Mapped(
                INTERRUPT_CONTROL_REG_ADDR,
                INTERRUPT_CONTROL_REG_ADDR + sizeof(regs) - 1,
                std::bind(&InterruptHandler::read8FromReg, this, std::placeholders::_1),
                std::bind(&InterruptHandler::externalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&InterruptHandler::read8FromReg, this, std::placeholders::_1),
                std::bind(&InterruptHandler::internalWrite8ToReg, this, std::placeholders::_1, std::placeholders::_2)));
    }

    bool InterruptHandler::isInterruptMasterSet() const
    {
        /*
        4000208h - IME - Interrupt Master Enable Register (R/W)

            Bit   Expl.
            0     Disable all interrupts         (0=Disable All, 1=See IE register)
            1-31  Not used

         */
        return le(regs.irqMasterEnable) & 0x1;
    }

    bool InterruptHandler::isCPSRInterruptSet() const
    {
        //  7     I - IRQ disable     (0=Enable, 1=Disable)                     ;
        return (cpu->state.accessReg(regs::CPSR_OFFSET) & (1 << 7)) == 0;
    }

    /*
    bool InterruptHandler::isCPSRFastInterruptSet() const
    {
        //  6     F - FIQ disable     (0=Enable, 1=Disable)                     ; Control
        return (cpu->state.accessReg(regs::CPSR_OFFSET) & (1 << 6)) == 0;
    }
    */

    bool InterruptHandler::isInterruptEnabled(InterruptType type) const
    {
        // 4000200h - IE - Interrupt Enable Register (R/W)
        return le(regs.irqEnable) & (0x1 << type);
    }

    void InterruptHandler::setInterrupt(InterruptType type)
    {
        regsMutex.lock();
        regs.irqRequest |= le(static_cast<uint16_t>(static_cast<uint16_t>(1) << type));
        regsMutex.unlock();
    }

    void InterruptHandler::checkForInterrupt()
    {
        uint16_t irqEnableReg = le(regs.irqEnable) & 0x3FFF;
        uint16_t irqRequestReg = le(regs.irqRequest) & 0x3FFF;

        // We can only execute the interrupt if all conditions are met
        if (isInterruptMasterSet() && isCPSRInterruptSet() && !needsOneIdleCycle && (irqEnableReg & irqRequestReg)) {

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
            cpu->state.decoder = &cpu->armDecoder;

            // Change the register mode to irq
            cpu->state.mode = CPUState::IRQ;

            // Ensure that the CPSR represents that we are in ARM mode again
            // Clear all flags & enforce irq mode
            // Also disable interrupts
            cpu->state.accessReg(regs::CPSR_OFFSET) = 0b010010 | (1 << 7);

            cpu->state.accessReg(regs::PC_OFFSET) = Memory::BIOS_IRQ_HANDLER_OFFSET;

            // Flush the pipeline
            cpu->initPipeline();

            // After irq we need to execute at least one instruction before another irq may be handled!
            needsOneIdleCycle = true;

        } else {
            //  No interrupt triggered -> at least one instruction executes
            needsOneIdleCycle = false;
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

} // namespace gbaemu