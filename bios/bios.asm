macro .word data{
    dw data
}

.word 0         ; 0x00
.word 0         ; 0x04
b swiHandler    ; 0x08
.word 0         ; 0x0C
.word 0         ; 0x10
.word 0         ; 0x14
b irqHandler    ; 0x18
.word 0         ; 0x1C

include 'swiHandler.asm'
include 'irqHandler.asm'

; Can be assembled with FASMARM: https://arm.flatassembler.net/