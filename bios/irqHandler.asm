irqHandler:
    ;Interrupt handler entry code
    stmfd r13!, {r0-r3,r12,r14}  ;save registers to SP_irq
    mov r0, 2 ; BIOS_DURING_IRQ (= 2)
    swi 0x2B0000 ; SWI to update bios state
    mov r0, 0x4000000 ;ptr+4 to 03FFFFFC (mirror of 03007FFC)
    add r14, r15, 0 ;retadr for USER handler $+8

    ldr r15, [r0, -4] ; call user irq handler

    ;Interrupt handler exit code
    mov r0, 3 ; BIOS_AFTER_IRQ (= 3)
    swi 0x2B0000 ; SWI to update bios state
    ldmfd r13!, {r0-r3,r12,r14}  ;restore registers from SP_irq
    subs r15, r14, 0 ;return from IRQ (PC=LR, CPSR=SPSR)
