CpuFastSet:
    ; safe used regs
    stmfd  sp!, {r3-r10, lr}

    ; is count > 0
    ; encode 0x1FFFFF by 0x200000 - 1
    mov r3, 0x200000
    sub r3, 1
    tst r2, r3
    beq cpuFastEnd ; eq means zero bit is set -> count = 0
    ; is memset?
    tst r2, 0x1000000
    ; extract count
    and r2, r3
    ; bit is set -> memset
    bne cpuFastMemset
cpuFastSetLoop:
    ldmia r0!, {r3-r10}
    stmia r1!, {r3-r10}
    subs r2, 8
    bgt cpuFastMemsetLoop
    b cpuFastEnd
cpuFastMemset:
    ; load the value for memset
    ldr r3, [r0]
    ; fill regs r2-r9 with the value of r3
    mov r4, r3
    mov r5, r3
    mov r6, r3
    mov r7, r3
    mov r8, r3
    mov r9, r3
    mov r10, r3
cpuFastMemsetLoop:
    ; write to desitnation
    stmia r1!, {r3-r10}
    subs r2, 8
    bgt cpuFastMemsetLoop
cpuFastEnd:
    ; restore regs, return & restore previous mode!
    ldmfd  sp!, {r3-r10, pc}
