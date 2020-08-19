#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "cpu.hpp"

int main(int argc, const char **argv)
{
    if (argc <= 1) {
        std::cout << "please provide a ROM file\n";
        return 0;
    }

    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

    std::cout << "read " << buf.size() << " bytes\n";

    gbaemu::CPU cpu;
    gbaemu::arm::ARMInstructionDecoder armDecoder;
    gbaemu::thumb::ThumbInstructionDecoder thumbDecoder;
    cpu.state.decoder = &armDecoder;
    cpu.state.memory.loadROM(reinterpret_cast<uint8_t *>(buf.data()), buf.size());
    //TODO are there conventions about inital reg values?
    cpu.state.accessReg(gbaemu::regs::PC_OFFSET) = 0x204;

    /*
        https://onlinedisassembler.com/odaweb/
        armv4 + force thumb yielded the best results
     */
    /*
    for (size_t i = 0x204; i < 0x204 + 20; ++i) {
        uint32_t read = mem[i];
        uint32_t flipped = (read & 0x000000FF) << 24 |
                           (read & 0x0000FF00) << 8 |
                           (read & 0x00FF0000) >> 8 |
                           (read & 0xFF000000) >> 24;
        std::cout << std::hex << flipped << std::endl;
        cpu.decode(read);
    }*/
    /*
    for (;;)
        cpu.step();
     */

    std::cout << "Game Title: ";
    for (size_t i = 0; i < 12; ++i) {
        std::cout << static_cast<char>(cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0A0 + i, nullptr));
    }
    std::cout << std::endl;
    std::cout << "Game Code: ";
    for (size_t i = 0; i < 4; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0AC + i, nullptr) << " ";
    }
    std::cout << std::endl;
    std::cout << "Maker Code: ";
    for (size_t i = 0; i < 2; ++i) {
        std::cout << std::hex << cpu.state.memory.read8(gbaemu::Memory::EXT_ROM_OFFSET + 0x0B0 + i, nullptr) << " ";
    }
    std::cout << std::endl;

    cpu.state.accessReg(gbaemu::regs::PC_OFFSET) = gbaemu::Memory::EXT_ROM_OFFSET;
    std::cout << cpu.state.disas(gbaemu::Memory::EXT_ROM_OFFSET, 200);

    cpu.step();
    cpu.step();
    cpu.step();

    std::cout << "========================================================================\n";

    std::cout << cpu.state.disas(gbaemu::Memory::EXT_ROM_OFFSET, 200);

    return 0;
}