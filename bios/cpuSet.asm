CpuSet:
    ; safe used regs
    stmfd  sp!, {r0-r3, lr}

    ; is count > 0
    ; encode 0x1FFFFF by 0x200000 - 1
    mov r3, 0x200000
    sub r3, 1
    tst r2, r3
    beq cpuEnd ; eq means zero bit is set -> count = 0
    ; is memset?
    tst r2, 0x1000000
    ; bit is set -> memset
    bne cpuMemset
    ; halfword or word?
    tst r2, 0x4000000
    ; extract count
    and r2, r3
    bne cpuSet32BitLoop
cpuSet16BitLoop:
    ldrh r3, [r0], 2
    strh r3, [r1], 2
    subs r2, 1
    bgt cpuSet16BitLoop
    b cpuEnd
cpuSet32BitLoop:
    ldmia r0!, {r3}
    stmia r1!, {r3}
    subs r2, 1
    bgt cpuSet32BitLoop
    b cpuEnd
cpuMemset:
    ; halfword or word?
    tst r2, 0x4000000
    ; extract count
    and r2, r3
    ; load the value for memset
    ldrhne r3, [r0]
    ldreq r3, [r0]
    bne cpuMemset32BitLoop
cpuMemset16BitLoop:
    strh r3, [r1], 2
    subs r2, 1
    bgt cpuMemset16BitLoop
    b cpuEnd
cpuMemset32BitLoop:
    ; write to desitnation
    stmia r1!, {r3}
    subs r2, 1
    bgt cpuMemset32BitLoop
cpuEnd:
    ; restore regs, return & restore previous mode!
    ldmfd  sp!, {r0-r3, pc}
