swiHandler:
    ; Each time when calling a BIOS function 4 words (SPSR, R11, R12, R14) are saved on Supervisor stack (_svc). Once it has saved that data, the SWI handler switches into System mode, so that all further stack operations are using user stack.
    stmfd sp!, {r11, r12, lr}
    ; save spsr to stack too
    mrs r12, spsr 
    stmfd sp!, {r12}

    ; Ugly hack to get the number of swi that should be called! :
    ; Reads the byte of the swi instruction which contains the number, because it is encoded to be readable from ARM & THUMB SWI call with the same offset
    ldrb r11, [lr, -2] ; r11 is not banked, but lr is! We need to read the swi number before changing the mode

    ; change to system mode with interrupt setting passthrough!
    and r12, 0x80 ; we only want to keep the T bit!
    orr r12, 0x1F ; Set mode to System Mode
    msr cpsr_c, r12 ; we only want to change the mode

;========= SYSTEM MODE =========

    ; we do not want to overwrite the users LR register -> push it
    stmfd sp!, {lr}

    ; set new return address to our exit code!
    mov lr, endSWIHandler

    ; Switch case for id
    cmp r11, 0x0B ; CpuSet
    moveq r12, CpuSet
    beq swiHandlerDefault ; break needed for no fall through!
    cmp r11, 0x0C ; CpuFasSet
    moveq r12, CpuFastSet
    ;TODO extend here if more bios code for SWIs is needed & do NOT FORGET BREAKS!!!

swiHandlerDefault:
    ; Default: not implemented with bios code -> eq condition is going to fail else we had a match -> call handler
    bxeq r12

endSWIHandler:
    ; restore user lr
    ldmfd sp!, {lr}

;========= END SYSTEM MODE =========

    ; we need to go back to supervisor mode(with disabled irqs & arm mode) and pop the previously pushed data!
    msr cpsr_c, 0x93

    ; Update the bios state
    mov r11, r0 ; backup r0
    mov r0, 1 ; BIOS_AFTER_SWI (= 1)
    swi 0x2B0000 ; SWI to update bios state
    mov r0, r11 ; restore r0

    ; now we can get back the previous spsr register
    ldmfd sp!, {r12}
    ; restore spsr register
    msr spsr, r12

    ; restore R11, R12, R14
    ldmfd sp!, {r11, r12, lr}

    ; finally we are able to exit the swi!
    movs pc, lr ; restore pc & cpsr register

include 'cpuFastSet.asm'
include 'cpuSet.asm'