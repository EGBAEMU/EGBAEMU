#include "swi.hpp"
#include "regs.hpp"

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>

namespace gbaemu
{
    namespace swi
    {

        void softReset(CPUState *state)
        {
            //TODO implement
        }
        void registerRamReset(CPUState *state)
        {
            //TODO implement
        }
        void halt(CPUState *state)
        {
            //TODO implement
        }
        void stop(CPUState *state)
        {
            //TODO implement
        }
        void intrWait(CPUState *state)
        {
            //TODO implement
        }
        void vBlankIntrWait(CPUState *state)
        {
            //TODO implement
        }

        /*
        Output:
            r0: numerator / denominator
            r1: numerator % denominator
            r3: abs(numerator / denominator)
        */
        static void _div(uint32_t *const *const currentRegs, int32_t numerator, int32_t denominator)
        {
            //TODO how to handle div by 0?

            int32_t div = numerator / denominator;

            *currentRegs[regs::R0_OFFSET] = static_cast<uint32_t>(div);
            // manually calculate remainder, because % isn't well defined for signed numbers across different platforms
            *currentRegs[regs::R1_OFFSET] = static_cast<uint32_t>(numerator - denominator * div);
            *currentRegs[regs::R3_OFFSET] = static_cast<uint32_t>(div < 0 ? -div : div);
        }
        void div(CPUState *state)
        {
            auto currentRegs = state->getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            _div(currentRegs, numerator, denominator);
        }

        void divArm(CPUState *state)
        {
            auto currentRegs = state->getCurrentRegs();

            int32_t numerator = static_cast<int32_t>(*currentRegs[regs::R1_OFFSET]);
            int32_t denominator = static_cast<int32_t>(*currentRegs[regs::R0_OFFSET]);
            _div(currentRegs, numerator, denominator);
        }

        void sqrt(CPUState *state)
        {
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            r0 = static_cast<int32_t>(std::sqrt(r0));
        }

        static double convertFromQ1_14ToFP(uint16_t fixedPnt)
        {
            uint16_t fixedPntPart = fixedPnt & 0x7FFF;
            double convertedFP = fixedPntPart / static_cast<double>(1 << 14);
            if (fixedPnt & 0x8000) {
                convertedFP = -convertedFP;
            }
            return convertedFP;
        }

        /*
        Calculates the arc tangent.
          r0   Tan, 16bit (1bit sign, 1bit integral part, 14bit decimal part)
        Return:
          r0   "-PI/2<THETA/<PI/2" in a range of C000h-4000h.
        Note: there is a problem in accuracy with "THETA<-PI/4, PI/4<THETA".
        */
        void arcTan(CPUState *state)
        {
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            double convertedFP = convertFromQ1_14ToFP(static_cast<uint16_t>(r0 & 0x0000FFFF));
            convertedFP = std::atan(convertedFP);
            bool negative = false;
            if (convertedFP < 0) {
                negative = true;
                convertedFP = -convertedFP;
            }

            // Upscale for conversion to fixed point
            convertedFP *= (1 << 14);
            // recreate fixed point format 16bit (1bit sign, 1bit integral part, 14bit decimal part)
            uint16_t fixedPntPart = (negative ? 0x8000 : 0x0000) | (static_cast<uint16_t>(convertedFP) & 0x7FFF);

            //TODO is there an interval reduction: -PI/2<THETA/<PI/2" in a range of C000h-4000h
            //TODO Q1.14 would probably be in a range of 0x0000-0x7FFF
            //TODO we probably need to fix this :/
            r0 = fixedPntPart;
        }

        //TODO what is meant by correction processing?
        /*
        Calculates the arc tangent after correction processing.
        Use this in normal situations.
          r0   X, 16bit (1bit sign, 1bit integral part, 14bit decimal part)
          r1   Y, 16bit (1bit sign, 1bit integral part, 14bit decimal part)
        Return:
          r0   0000h-FFFFh for 0<=THETA<2PI.
        */
        void arcTan2(CPUState *state)
        {
            uint32_t &r0 = state->accessReg(regs::R0_OFFSET);
            double x = convertFromQ1_14ToFP(static_cast<uint16_t>(r0 & 0x0000FFFF));
            double y = convertFromQ1_14ToFP(static_cast<uint16_t>(state->accessReg(regs::R1_OFFSET) & 0x0000FFFF));

            //TODO atan2 returns [-pi/2,pi/2] but we need [0, 2pi]... can we just make this interval transformation or is it calculated differently???
            double res = std::atan2(x, y) * 2 + M_PI;
            // Transform to integer interval [0, 0xFFFF]
            r0 = static_cast<uint32_t>(static_cast<uint16_t>((res * 0x0FFFF) / (2 * M_PI)));
        }

        //TODO Note: On GBA, NDS7 and DSi7, these two functions(cpuset & cpufastset) will silently reject to do anything if the source start or end addresses are reaching into the BIOS area. The NDS9 and DSi9 don't have such read-proctections.
        /*
        Memory copy/fill in units of 32 bytes. Memcopy is implemented as repeated LDMIA/STMIA [Rb]!,r2-r9 instructions. Memfill as single LDR followed by repeated STMIA [Rb]!,r2-r9.
        After processing all 32-byte-blocks, the NDS/DSi additonally processes the remaining words as 4-byte blocks. BUG: The NDS/DSi uses the fast 32-byte-block processing only for the first N bytes (not for the first N words), so only the first quarter of the memory block is FAST, the remaining three quarters are SLOWLY copied word-by-word.
        The length is specifed as wordcount, ie. the number of bytes divided by 4.
        On the GBA, the length should be a multiple of 8 words (32 bytes) (otherwise the GBA is forcefully rounding-up the length). On NDS/DSi, the length may be any number of words (4 bytes).
          r0    Source address        (must be aligned by 4)
          r1    Destination address   (must be aligned by 4)
          r2    Length/Mode
                  Bit 0-20  Wordcount (GBA: rounded-up to multiple of 8 words)
                  Bit 24    Fixed Source Address (0=Copy, 1=Fill by WORD[r0])
        Return: No return value, Data written to destination address.
        */
        void cpuFastSet(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint8_t *sourceAddr = state->memory.resolveAddr(*currentRegs[regs::R0_OFFSET]);
            uint8_t *destAddr = state->memory.resolveAddr(*currentRegs[regs::R1_OFFSET]);
            uint32_t length_mode = *currentRegs[regs::R2_OFFSET];

            uint32_t length = length_mode & 0x001FFFFF;
            uint32_t rest = length % 8;
            // ceiling to multiple of 8
            if (rest) {
                length += 8 - rest;
            }
            // convert from word(32 bit) count to byte count
            length <<= 2;

            bool fixedMode = length_mode & (1 << 24);
            if (fixedMode) {
                // Fill with value pointed to by r0
                uint32_t value = *reinterpret_cast<uint32_t *>(sourceAddr);
                uint32_t *destPtr = reinterpret_cast<uint32_t *>(destAddr);
                uint32_t *endPtr = destPtr + length / sizeof(*destPtr);
                std::fill(destPtr, endPtr, value);
            } else {
                // Normal memcpy behaviour
                std::memcpy(destAddr, sourceAddr, length);
            }
        }
        /*
        Memory copy/fill in units of 4 bytes or 2 bytes. Memcopy is implemented as repeated LDMIA/STMIA [Rb]!,r3 or LDRH/STRH r3,[r0,r5] instructions. Memfill as single LDMIA or LDRH followed by repeated STMIA [Rb]!,r3 or STRH r3,[r0,r5].
        The length must be a multiple of 4 bytes (32bit mode) or 2 bytes (16bit mode). The (half)wordcount in r2 must be length/4 (32bit mode) or length/2 (16bit mode), ie. length in word/halfword units rather than byte units.
          r0    Source address        (must be aligned by 4 for 32bit, by 2 for 16bit)
          r1    Destination address   (must be aligned by 4 for 32bit, by 2 for 16bit)
          r2    Length/Mode
                  Bit 0-20  Wordcount (for 32bit), or Halfwordcount (for 16bit)
                  Bit 24    Fixed Source Address (0=Copy, 1=Fill by {HALF}WORD[r0])
                  Bit 26    Datasize (0=16bit, 1=32bit)
        Return: No return value, Data written to destination address.
        */
        void cpuSet(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint8_t *sourceAddr = state->memory.resolveAddr(*currentRegs[regs::R0_OFFSET]);
            uint8_t *destAddr = state->memory.resolveAddr(*currentRegs[regs::R1_OFFSET]);
            uint32_t length_mode = *currentRegs[regs::R2_OFFSET];

            uint32_t length = length_mode & 0x001FFFFF;
            bool fixedMode = length_mode & (1 << 24);
            bool dataSize32bit = length_mode & (1 << 26);

            length <<= dataSize32bit ? 2 : 1;

            if (fixedMode) {
                // Fill with value pointed to by r0
                if (dataSize32bit) {
                    uint32_t value = *reinterpret_cast<uint32_t *>(sourceAddr);
                    uint32_t *destPtr = reinterpret_cast<uint32_t *>(destAddr);
                    uint32_t *endPtr = destPtr + length / sizeof(*destPtr);
                    std::fill(destPtr, endPtr, value);
                } else {
                    uint16_t value = *reinterpret_cast<uint16_t *>(sourceAddr);
                    uint16_t *destPtr = reinterpret_cast<uint16_t *>(destAddr);
                    uint16_t *endPtr = destPtr + length / sizeof(*destPtr);
                    std::fill(destPtr, endPtr, value);
                }
            } else {
                // Normal memcpy behaviour
                std::memcpy(destAddr, sourceAddr, length);
            }
        }

        /*
        Calculates the checksum of the BIOS ROM (by reading in 32bit units, and adding up these values). IRQ and FIQ are disabled during execution.
        The checksum is BAAE187Fh (GBA and GBA SP), or BAAE1880h (NDS/3DS in GBA mode, whereas the only difference is that the byte at [3F0Ch] is changed from 00h to 01h, otherwise the BIOS is 1:1 same as GBA BIOS, it does even include multiboot code).
        Parameters: None. Return: r0=Checksum.
        */
        void biosChecksum(CPUState *state)
        {
            state->accessReg(regs::R0_OFFSET) = 0x0BAAE18F;
        }

        void bgAffineSet(CPUState *state)
        {
            //TODO implement
        }
        void objAffineSet(CPUState *state)
        {
            //TODO implement
        }

        /*
        BitUnPack - SWI 10h (GBA/NDS7/NDS9/DSi7/DSi9)
        Used to increase the color depth of bitmaps or tile data. For example, to convert a 1bit monochrome font into 4bit or 8bit GBA tiles. The Unpack Info is specified separately, allowing to convert the same source data into different formats.
          r0  Source Address      (no alignment required)
          r1  Destination Address (must be 32bit-word aligned)
          r2  Pointer to UnPack information:
               16bit  Length of Source Data in bytes     (0-FFFFh)
               8bit   Width of Source Units in bits      (only 1,2,4,8 supported)
               8bit   Width of Destination Units in bits (only 1,2,4,8,16,32 supported)
               32bit  Data Offset (Bit 0-30), and Zero Data Flag (Bit 31)
              The Data Offset is always added to all non-zero source units.
              If the Zero Data Flag was set, it is also added to zero units.
        Data is written in 32bit units, Destination can be Wram or Vram. The size of unpacked data must be a multiple of 4 bytes. The width of source units (plus the offset) should not exceed the destination width.
        Return: No return value, Data written to destination address
        */
        void bitUnPack(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint8_t *sourceAddr = state->memory.resolveAddr(*currentRegs[regs::R0_OFFSET]);
            uint8_t *destAddr = state->memory.resolveAddr(*currentRegs[regs::R1_OFFSET]);
            uint32_t unpackFormatPtr = *currentRegs[regs::R2_OFFSET];

            uint16_t srcByteCount = state->memory.read16(unpackFormatPtr);
            uint8_t srcUnitWidth = state->memory.read8(unpackFormatPtr + 2);
            uint8_t destUnitWidth = state->memory.read8(unpackFormatPtr + 3);
            uint32_t dataOffset = state->memory.read32(unpackFormatPtr + 4);
            bool zeroDataOff = dataOffset & (1 << 31);
            dataOffset &= 0x7FFFFFF;

            //TODO implement unpacking... not sure how this is intended
        }

        /*
        LZ77UnCompReadNormalWrite8bit (Wram) - SWI 11h (GBA/NDS7/NDS9/DSi7/DSi9)
        LZ77UnCompReadNormalWrite16bit (Vram) - SWI 12h (GBA)
        LZ77UnCompReadByCallbackWrite8bit - SWI 01h (DSi7/DSi9)
        LZ77UnCompReadByCallbackWrite16bit - SWI 12h (NDS), SWI 02h or 19h (DSi)
        Expands LZ77-compressed data. The Wram function is faster, and writes in units of 8bits. 
        For the Vram function the destination must be halfword aligned, data is written in units of 16bits.
        CAUTION: Writing 16bit units to [dest-1] instead of 8bit units to [dest] means that reading from [dest-1] won't work, 
        ie. the "Vram" function works only with disp=001h..FFFh, but not with disp=000h.
        If the size of the compressed data is not a multiple of 4, please adjust it as much as possible by padding with 0.
        Align the source address to a 4-Byte boundary.
          r0  Source address, pointing to data as such:
               Data header (32bit)
                 Bit 0-3   Reserved
                 Bit 4-7   Compressed type (must be 1 for LZ77)
                 Bit 8-31  Size of decompressed data
               Repeat below. Each Flag Byte followed by eight Blocks.
               Flag data (8bit)
                 Bit 0-7   Type Flags for next 8 Blocks, MSB first
               Block Type 0 - Uncompressed - Copy 1 Byte from Source to Dest
                 Bit 0-7   One data byte to be copied to dest
               Block Type 1 - Compressed - Copy N+3 Bytes from Dest-Disp-1 to Dest
                 Bit 0-3   Disp MSBs
                 Bit 4-7   Number of bytes to copy (minus 3)
                 Bit 8-15  Disp LSBs
          r1  Destination address
          r2  Callback parameter        ;\for NDS/DSi "ReadByCallback" variants only
          r3  Callback structure        ;/(see Callback notes below)
        Return: No return value.
        */
        //TODO is the difference between writing in units of 8 bit vs 16 bit relevant?
        static void _LZ77UnComp(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            const uint32_t dataHeader = state->memory.read32(sourceAddr);
            sourceAddr += 4;

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;
            uint32_t decompressedSize = (dataHeader >> 8) & 0x00FFFFFF;

            // Value should be 3 for run-length decompression
            if (compressedType != 1) {
                std::cerr << "ERROR: Invalid call of LZ77UnComp!" << std::endl;
                return;
            }

            while (decompressedSize > 0) {
                const uint8_t typeBitset = state->memory.read8(sourceAddr++);

                // process each block
                for (uint8_t i = 0; i < 8; ++i) {
                    bool type1 = (typeBitset >> (7 - i)) & 0x1;

                    if (type1) {
                        // Type 1 uses previously written data as lookup source
                        uint16_t type1Desc = state->memory.read16(sourceAddr);
                        sourceAddr += 2;

                        uint16_t disp = (((type1Desc & 0x0F) << 8) | ((type1Desc >> 8) & 0x0FF)) + 1;
                        uint8_t n = ((type1Desc >> 4) & 0x0F) + 3;

                        // We read & write n bytes of uncompressed data
                        decompressedSize -= n;

                        // Copy N Bytes from Dest-Disp to Dest (+3 and - 1 already applied)
                        uint32_t readAddr = destAddr - disp;
                        while (n > 0) {
                            state->memory.write8(destAddr++, state->memory.read8(readAddr++));
                        }
                    } else {
                        // Type 0 is one uncompressed byte of data
                        uint8_t data = state->memory.read8(sourceAddr++);
                        --decompressedSize;
                        state->memory.write8(destAddr++, data);
                    }
                }
            }
        }

        void LZ77UnCompWRAM(CPUState *state)
        {
            _LZ77UnComp(state);
        }
        void LZ77UnCompVRAM(CPUState *state)
        {
            _LZ77UnComp(state);
        }

        /*
        HuffUnCompReadNormal - SWI 13h (GBA)
        HuffUnCompReadByCallback - SWI 13h (NDS/DSi)
        The decoder starts in root node, the separate bits in the bitstream specify if the next node is node0 or node1, if that node is a data node, then the data is stored in memory, and the decoder is reset to the root node. The most often used data should be as close to the root node as possible. For example, the 4-byte string "Huff" could be compressed to 6 bits: 10-11-0-0, with root.0 pointing directly to data "f", and root.1 pointing to a child node, whose nodes point to data "H" and data "u".
        Data is written in units of 32bits, if the size of the compressed data is not a multiple of 4, please adjust it as much as possible by padding with 0.
        Align the source address to a 4Byte boundary.
          r0  Source Address, aligned by 4, pointing to:
               Data Header (32bit)
                 Bit0-3   Data size in bit units (normally 4 or 8)
                 Bit4-7   Compressed type (must be 2 for Huffman)
                 Bit8-31  24bit size of decompressed data in bytes
               Tree Size (8bit)
                 Bit0-7   Size of Tree Table/2-1 (ie. Offset to Compressed Bitstream)
               Tree Table (list of 8bit nodes, starting with the root node)
                Root Node and Non-Data-Child Nodes are:
                 Bit0-5   Offset to next child node,
                          Next child node0 is at (CurrentAddr AND NOT 1)+Offset*2+2
                          Next child node1 is at (CurrentAddr AND NOT 1)+Offset*2+2+1
                 Bit6     Node1 End Flag (1=Next child node is data)
                 Bit7     Node0 End Flag (1=Next child node is data)
                Data nodes are (when End Flag was set in parent node):
                 Bit0-7   Data (upper bits should be zero if Data Size is less than 8)
               Compressed Bitstream (stored in units of 32bits)
                 Bit0-31  Node Bits (Bit31=First Bit)  (0=Node0, 1=Node1)
          r1  Destination Address
          r2  Callback temp buffer      ;\for NDS/DSi "ReadByCallback" variants only
          r3  Callback structure        ;/(see Callback notes below)
        Return: No return value, Data written to destination address.
        */
        void huffUnComp(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            uint32_t dataHeader = state->memory.read32(sourceAddr);
            sourceAddr += 4;

            uint32_t decompressedBits = ((dataHeader >> 8) & 0x00FFFFFF) * 8;
            //TODO do we need to add 1 for the correct size, else are 16bit data impossible and 0 bit data does not make sense
            const uint8_t dataSize = dataHeader & 0x0F;

            // data size should be a multiple of 4
            if (dataSize % 4) {
                std::cerr << "WARNING: huffman decompression data is not a multiple of 4 bit! PLS try to add 1 to the dataSize." << std::endl;
            }

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;

            // Value should be 2 for huffman
            if (compressedType != 2) {
                std::cerr << "ERROR: Invalid call of huffUnComp!" << std::endl;
                return;
            }

            uint8_t treeSize = state->memory.read8(sourceAddr);
            sourceAddr += 1;

            const uint32_t treeRoot = sourceAddr;
            sourceAddr += treeSize;

            // data is should be written in 32 bit batches, therefore we have to buffer the decompressed data and keep track of the left space
            uint32_t writeBuf = 0;
            uint8_t writeBufOffset = 0;

            // as bits needed for decompressions varies we need to keep track of bits left in the read buffer
            uint32_t readBuf = state->memory.read32(sourceAddr);
            sourceAddr += 4;
            uint8_t readBufBitsLeft = 32;

            //TODO do we need to fix things if 32 % dataSize != 0?
            if (32 % dataSize) {
                std::cerr << "WARNING: decompressed huffman data might be misaligned, if not pls remove this warning and if so, well FML!" << std::endl;
            }

            for (; decompressedBits > 0; decompressedBits -= dataSize) {
                uint32_t currentParsingAddr = treeRoot;
                bool isDataNode = false;

                // Bit wise tree walk
                for (;;) {
                    uint8_t node = state->memory.read8(currentParsingAddr);

                    if (isDataNode) {
                        writeBuf |= (static_cast<uint32_t>(node) << writeBufOffset);
                        writeBufOffset += dataSize;
                        break;
                    }

                    // We have a parent node so lets check for the next node and if it is a data node
                    uint8_t offset = node & 0x1F;
                    bool isNode1EndFlag = (node >> 6) & 0x1;
                    bool isNode0EndFlag = (node >> 7) & 0x1;
                    --readBufBitsLeft;
                    bool decompressBit = (readBuf >> readBufBitsLeft) & 0x1;

                    isDataNode = decompressBit ? isNode1EndFlag : isNode0EndFlag;
                    //TODO the calculation of the next node might be wrong...
                    currentParsingAddr = (currentParsingAddr & (~static_cast<uint32_t>(1))) + static_cast<uint32_t>(offset) * 2 + (decompressBit ? 3 : 2);

                    // Fill empty read buffer again
                    if (readBufBitsLeft == 0) {
                        readBuf = state->memory.read32(sourceAddr);
                        sourceAddr += 4;
                        readBufBitsLeft = 32;
                    }
                }

                // Is there is no more space left for decompressed data or we are done decompressing(only dataSize bits left) then we have to flush our buffer
                if (writeBufOffset + dataSize > 32 || decompressedBits == dataSize) {
                    state->memory.write32(destAddr, writeBuf);
                    destAddr += 4;
                    // Reset buf state
                    writeBufOffset = 0;
                    writeBuf = 0;
                }
            }
        }

        /*
        RLUnCompReadNormalWrite8bit (Wram) - SWI 14h (GBA/NDS7/NDS9/DSi7/DSi9)
        RLUnCompReadNormalWrite16bit (Vram) - SWI 15h (GBA)
        RLUnCompReadByCallbackWrite16bit - SWI 15h (NDS7/NDS9/DSi7/DSi9)
        Expands run-length compressed data. The Wram function is faster, and writes in units of 8bits. 
        For the Vram function the destination must be halfword aligned, data is written in units of 16bits.
        If the size of the compressed data is not a multiple of 4, please adjust it as much as possible by padding with 0. Align the source address to a 4Byte boundary.
          r0  Source Address, pointing to data as such:
               Data header (32bit)
                 Bit 0-3   Reserved
                 Bit 4-7   Compressed type (must be 3 for run-length)
                 Bit 8-31  Size of decompressed data
               Repeat below. Each Flag Byte followed by one or more Data Bytes.
               Flag data (8bit)
                 Bit 0-6   Expanded Data Length (uncompressed N-1, compressed N-3)
                 Bit 7     Flag (0=uncompressed, 1=compressed)
               Data Byte(s) - N uncompressed bytes, or 1 byte repeated N times
          r1  Destination Address
          r2  Callback parameter        ;\for NDS/DSi "ReadByCallback" variants only
          r3  Callback structure        ;/(see Callback notes below)
        Return: No return value, Data written to destination address.
        */
        //TODO is the difference between writing in units of 8 bit vs 16 bit relevant?
        static void _rlUnComp(CPUState *state)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint32_t sourceAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            const uint32_t dataHeader = state->memory.read32(sourceAddr);
            sourceAddr += 4;

            const uint8_t compressedType = (dataHeader >> 4) & 0x0F;
            uint32_t decompressedSize = (dataHeader >> 8) & 0x00FFFFFF;

            // Value should be 3 for run-length decompression
            if (compressedType != 3) {
                std::cerr << "ERROR: Invalid call of rlUnComp!" << std::endl;
                return;
            }

            while (decompressedSize > 0) {
                uint8_t flagData = state->memory.read8(sourceAddr++);

                bool compressed = (flagData >> 7) & 0x1;
                uint8_t decompressedDataLength = (flagData & 0x7F) + (compressed ? 3 : 1);

                if (decompressedSize < decompressedDataLength) {
                    std::cerr << "ERROR: underflow in rlUnComp!" << std::endl;
                    return;
                }
                decompressedSize -= decompressedDataLength;

                uint8_t data = state->memory.read8(sourceAddr++);

                // write read data byte N times for decompression
                for (; decompressedDataLength > 0; --decompressedDataLength) {
                    state->memory.write8(destAddr++, data);
                }
            }
        }

        void RLUnCompWRAM(CPUState *state)
        {
            _rlUnComp(state);
        }
        void RLUnCompVRAM(CPUState *state)
        {
            _rlUnComp(state);
        }

        /*
        Diff8bitUnFilterWrite8bit (Wram) - SWI 16h (GBA/NDS9/DSi9)
        Diff8bitUnFilterWrite16bit (Vram) - SWI 17h (GBA)
        Diff16bitUnFilter - SWI 18h (GBA/NDS9/DSi9)
        These aren't actually real decompression functions, destination data will have exactly the same size as source data. However, assume a bitmap or wave form to contain a stream of increasing numbers such like 10..19, the filtered/unfiltered data would be:
          unfiltered:   10  11  12  13  14  15  16  17  18  19
          filtered:     10  +1  +1  +1  +1  +1  +1  +1  +1  +1
        In this case using filtered data (combined with actual compression algorithms) will obviously produce better compression results.
        Data units may be either 8bit or 16bit used with Diff8bit or Diff16bit functions respectively.
          r0  Source address (must be aligned by 4) pointing to data as follows:
               Data Header (32bit)
                 Bit 0-3   Data size (must be 1 for Diff8bit, 2 for Diff16bit)
                 Bit 4-7   Type (must be 8 for DiffFiltered)
                 Bit 8-31  24bit size after decompression
               Data Units (each 8bit or 16bit depending on used SWI function)
                 Data0          ;original data
                 Data1-Data0    ;difference data
                 Data2-Data1    ;...
                 Data3-Data2
                 ...
          r1  Destination address
        Return: No return value, Data written to destination address.
        */
        static void _diffUnFilter(CPUState *state, bool bits8)
        {
            const auto currentRegs = state->getCurrentRegs();
            uint32_t srcAddr = *currentRegs[regs::R0_OFFSET];
            uint32_t destAddr = *currentRegs[regs::R1_OFFSET];

            uint32_t info = state->memory.read32(srcAddr);
            srcAddr += 4;

            //TODO not sure if we need those... maybe for sanity checks
            /*uint8_t dataSize = info & 0x0F;
            uint8_t type = (info >> 4) & 0x0F;*/

            uint32_t size = (info >> 8) & 0x00FFFFFF;

            uint8_t addressInc = bits8 ? 1 : 2;

            uint16_t current = 0;
            do {
                uint16_t diff = bits8 ? state->memory.read8(srcAddr) : state->memory.read16(srcAddr);
                current += diff;
                bits8 ? state->memory.write8(destAddr, static_cast<uint8_t>(current & 0x0FF)) : state->memory.write16(srcAddr, current);
                destAddr += addressInc;
                srcAddr += addressInc;
            } while (--size);
        }

        void diff8BitUnFilterWRAM(CPUState *state)
        {
            _diffUnFilter(state, true);
        }
        void diff8BitUnFilterVRAM(CPUState *state)
        {
            _diffUnFilter(state, true);
        }
        void diff16BitUnFilter(CPUState *state)
        {
            _diffUnFilter(state, false);
        }

        void soundBiasChange(CPUState *state)
        {
            //TODO implement
        }
        void soundDriverInit(CPUState *state)
        {
            //TODO implement
        }
        void soundDriverMode(CPUState *state)
        {
            //TODO implement
        }
        void soundDriverMain(CPUState *state)
        {
            //TODO implement
        }
        void soundDriverVSync(CPUState *state)
        {
            //TODO implement
        }
        void soundChannelClear(CPUState *state)
        {
            //TODO implement
        }
        void MIDIKey2Freq(CPUState *state)
        {
            //TODO implement
        }
        void musicPlayerOpen(CPUState *state)
        {
            //TODO implement
        }
        void musicPlayerStart(CPUState *state)
        {
            //TODO implement
        }
        void musicPlayerStop(CPUState *state)
        {
            //TODO implement
        }
        void musicPlayerContinue(CPUState *state)
        {
            //TODO implement
        }
        void musicPlayerFadeOut(CPUState *state)
        {
            //TODO implement
        }
        void multiBoot(CPUState *state)
        {
            //TODO implement
        }
        void hardReset(CPUState *state)
        {
            //TODO implement
        }
        void customHalt(CPUState *state)
        {
            //TODO implement
        }
        void soundDriverVSyncOff(CPUState *state)
        {
            //TODO implement
        }
        void soundDriverVSyncOn(CPUState *state)
        {
            //TODO implement
        }
        void getJumpList(CPUState *state)
        {
            //TODO implement
        }

    } // namespace swi
} // namespace gbaemu