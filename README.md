# Gameboy Advance Simulator
Within the ESCHO1 final project our group implemented an emulator for the Gabeboy Advance.

## Project Status
The current status includes the following components:
 
 - The 2 instruction sets, ARMv4 and Thumb
 - The 4 DMA controllers
 - The 4 available timers
 - The LCD rendering
 - Interrupt processing

With this, most games can already be emulated without problems. 
For reference we have taken the _Super Mario Advance_ games here. 
As open, but not essential points remain:

 - The 6 audio channels (For 2 of the channels a functional start has already been made in the corresponding branch. However, this is not optimized).
 - Coprocessor functionality

## Building the emulator
For compiling the emulator, [SDL2](https://www.libsdl.org/index.php) is required first. 
If this requirement is met, the provided Makefile 

Translated with www.DeepL.com/Translator (free version)