#include "interrupts.hpp"

namespace gbaemu {

    bool InterruptHandler::isInterruptMasterSet() const 
    {   
        /*
        4000208h - IME - Interrupt Master Enable Register (R/W)

            Bit   Expl.
            0     Disable all interrupts         (0=Disable All, 1=See IE register)
            1-31  Not used

         */
        InstructionExecutionInfo info{0};
        return cpu->state.memory.read32(0x4000208, &info) & 0x1;
    }

    bool InterruptHandler::isCPSRInterruptSet() const
    {
        //  7     I - IRQ disable     (0=Enable, 1=Disable)                     ;
        return (cpu->state.accessReg(regs::CPSR_OFFSET) & (0x1 << 8)) == 0;
    }

    bool InterruptHandler::isCPSRFastInterruptSet() const
    {
        //  6     F - FIQ disable     (0=Enable, 1=Disable)                     ; Control
        return (cpu->state.accessReg(regs::CPSR_OFFSET) & (0x1 << 7)) == 0;
    }

    bool InterruptHandler::isInterruptEnabled(InterruptType type) const
    {
        InstructionExecutionInfo info{0};
        // 4000200h - IE - Interrupt Enable Register (R/W)
        return cpu->state.memory.read16(0x4000200, &info) & (0x1 << type);
    }

    bool InterruptHandler::wasInterruptAcknowledged(InterruptType type) const
    {   
        InstructionExecutionInfo info{0};
        // 4000202h - IF - Interrupt Request Flags / IRQ Acknowledge (R/W, see below)
        // Interrupts must be manually acknowledged by writing a "1" to one of the 
        // IRQ bits, the IRQ bit will then be cleared.
        return cpu->state.memory.read16(0x4000202, &info) & (0x1 << type);
    }

    void InterruptHandler::checkForInterrupt() const 
    {   
        // We can only execute the interrupt if all conditions are met
        if(isInterruptMasterSet() && isCPSRInterruptSet()) {
          
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
            // Save PC+4 to LR_irq
            //TODO PC or PC + (2/4)
            *(cpu->state.getModeRegs(CPUState::IRQ)[regs::LR_OFFSET]) = cpu->state.getCurrentPC();

            // Change instruction mode to arm
            cpu->state.decoder = &cpu->armDecoder;
            // Ensure that the CPSR represents that we are in ARM mode again
            // Clear all flags & enforce irq mode
            cpu->state.accessReg(regs::CPSR_OFFSET) = 0b010010;

            // Change the register mode to irq
            cpu->state.mode = CPUState::IRQ;

            cpu->state.accessReg(regs::PC_OFFSET) = cpu->state.memory.getBiosBaseAddr() + Memory::BIOS_IRQ_HANDLER_OFFSET;

            // Flush the pipeline
            cpu->initPipeline();

        }
    }

}