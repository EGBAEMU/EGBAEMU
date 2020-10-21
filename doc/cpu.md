# GBA CPU emulation
The Gameboy advance uses a ARM7TDMI CPU(32 bit RISC), running at 16.78 MHz.
It supports 2 distinct instruction sets: ARMv4 and Thumb.
The first one has a instruction width of 32 bit, whereas the latter one only needs 16 bit. 
Thus Thumb may be seen as a much more condensed subset of the ARMv4 instruction set, therefore most thumb instruction implementaions are forwarded to arm instruction implementations.
Specifics on the two different instruction sets and the register set may be found [here](https://problemkaputt.de/gbatek.htm).

## Implementation details
In the very first implementation decode and execute stage where cleanly separated, with a datastructure representing a decoded instruction.
However the performance did suffer from this distinction.  

The second implementation forwarded known attributes, i.e. instruction category and instruction, from decoding stage directly into execution stage via templates.
The decoding stage and execution stage where merged by calling the appropiate instruction handler within the decode method.

Finally the approach used by [eggvance](https://smolka.dev/eggvance/progress-3/) was adopted for minimal branching and decoding overhead, using a Look-Up-Table (LUT) and extreme templating to forward even more known attributs, i.e. if an immediate or register offset is used.

Another very important optimizations were:
- transforming the step function from 1 cycle execution to x cycle execution at once
- ["state dependtend dispatching"](https://smolka.dev/eggvance/progress-5/)