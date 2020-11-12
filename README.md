# Gameboy Advance Simulator
Within the ESHO1 final project our group implemented an emulator for the Gameboy Advance.

## Project Status
The current project status includes the following components:
 
 - 2 instruction sets, ARMv4 and Thumb
 - 4 DMA channel
 - 4 Timer channel
 - LCD rendering
 - Interrupt processing
 - external bios rom support
 - (buggy) fallback bios

With this, most games can already be emulated without problems.
The following open points remain:

 - The 6 audio channels (For 2 of the channels a functional start has already been made in the corresponding branch. However, this is not optimized at all).
 - Real Time Clock (RTC) support

## Building the emulator
For compiling the emulator, [SDL2](https://www.libsdl.org/index.php) and gcc are required. 
If those requirements are met, the provided Makefile can be used for building.

### Linux
On most distributions SDL2 can be installed via the packet manager, i.e. for Debian:
> ``
apt-get install libsdl2-dev
``

Building:

> ``
make
``
### Windows
#### MinGW
Download SDL2 MinGW version from [here](https://www.libsdl.org/download-2.0.php).

> ``
make
``

#### MSVC
Download SDL2 Visual C++ version from [here](https://www.libsdl.org/download-2.0.php).

> ``
make windows
``

or to avoid a complete rebuild (after initial `make windows`)

> ``
make windows_
``

### Configuring Builds
Several flags can be set to enable debug features. 
In the `src/logging.hpp` source file debug output for various submodules can be adjusted.
These are:

| Flag       | Debug target / Meaning |
|------------|------------|
| LEGACY_RENDERING | Use legacy rendering mode (might be more stable, but slower) |
| DEBUG_CLI  | Enables interactive debugging tools (see `src/debugger.cpp` for available commands) |
| DUMP_CPU_STATE  | only usable with DEBUG_CLI, dumps the cpu state onto the console after each step, highly recommended to pipe into a file |
| DEBUG_DMA  | DMA emulation |
| DEBUG_IRQ  | Interrupt emulation |
| DEBUG_TIM  | Timer emulation |
| DEBUG_LCD  | LCD rendering |
| DEBUG_MEM  | Memory management |
| DEBUG_IO   | Handling of I/O address space |
| DEBUG_SAVE | Save file handling |
| DEBUG_SWI  | Software interrupt emulation |

In `src/main.cpp` the following additional flags may be adjusted:
| Flag       | Meaning |
|------------|------------|
| LIMIT_FPS  | Caps FPS to 60 (considers only time needed for current frame) |
| PRINT_FPS  | Prints the current FPS (per frame) onto the console |
| DUMP_ROM   | Dumps the dissassembled rom in ARM and Thumb mode onto the console prior to emulation, highly recommended to pipe into a file! |

In `src/lcd/defs.hpp`
| Flag       | Meaning |
|------------|------------|
| RENDERER_DECOMPOSE_LAYERS | show each layer, useful for graphical debugging |
| RENDERER_DECOMPOSE_BG_COLOR | replaces transparent color with specified one |
| RENDERER_USE_FB_CANVAS | for usage of a frame buffer for showing the image, probably wrong color format in master branch, see rpi branch. This also affects the usage: first argument is expected to be a path to the frame buffer, i.e. `/dev/fb1` |

## Using the emulator
The emulator may be used via the console as follows

> ``
./gbaemu rom [bios.rom]
``

Although the usage of an external bio rom is not required, it is highly recommended as there are known bugs in the fallback solution (i.e. decompression) and no time to fix those (yet).

Save file is automatically generated with appended `.sav` to the whole rom name: i.e. for `rom.gba` the resulting save file would be `rom.gba.sav`.

### Keymap
The keymap is not configurable during runtime, but can be adjusted by modifying the `std::map` in `src/input/keyboard_control.hpp`.
| GBA Button | Key mapping |
|------------|-------------|
| A | Space, j |
| B | k, left shift |
| Select | esc |
| Start | enter |
| L | l |
| R | colon, p |
| Up | w |
| Down | s |
| Left | a |
| Right | d |

## Implementation details

Specifics on the implementation are to be found in the subdirectory `doc`.

## External Ressources
- For general GBA info: [GBATEK](https://problemkaputt.de/gbatek.htm)
- For Audio: http://belogic.com/gba/
- Further documentation [GBADEV](https://www.gbadev.org/docs.php)
- Demos [GBADEV](https://www.gbadev.org/demos.php)
- Eggvance [Blog](https://smolka.dev/eggvance/)
- mGBA [Blog](https://mgba.io/tag/development/)
- mGBA [test suite](https://github.com/mgba-emu/suite)
- eggvance [test suite](https://github.com/jsmolka/gba-suite)

## Other (more advance, stable & maintained) Emulators
- [mGBA](https://github.com/mgba-emu/mgba)
- [eggvance](https://github.com/jsmolka/eggvance)
- [no$gba](https://problemkaputt.de/gba.htm)
- and many [more](https://www.gbadev.org/tools.php?section=Emulator)
