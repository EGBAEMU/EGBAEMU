# GBA Fallback Bios Emulation
> Note: the fallback bios contains known bugs, please use an external bios during emulation.

The fallback bios implementation can be found at several places:
- `bios/` contains commented assembly code for (hard- & software) interrupt handling and implementations of GBA memcpy (cpuFastSet, cpuSet). Those files can be compiled using [FASMARM](https://arm.flatassembler.net/) (apply compiler to bios.asm as this is the root of all other files). `readBin.cpp` can then be used to generate the c/c++ byte array used as emulation base(in fallback mode) see: `src/io/bios.cpp`.
- `src/cpu/swi.cpp` contains fallback implementations for some system calls. See summary at [GBATEK](https://problemkaputt.de/gbatek.htm#biosfunctions). It also contains a method (callBiosCodeSWIHandler) to call a SWI Handler inside a bios file (as always for an external bios or in fallback mode for GBA memcpy versions).