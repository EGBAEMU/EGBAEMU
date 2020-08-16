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
    gbaemu::ARMInstructionDecoder armDecoder;
    cpu.state.decoder = &armDecoder;
    cpu.state.memory.mem = buf.data();
    cpu.state.memory.memSize = buf.size();
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
    for (;;)
        cpu.step();

    return 0;
}