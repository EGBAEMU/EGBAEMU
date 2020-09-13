#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char **argv)
{
    if (argc <= 1) {
        std::cout << "please provide a bin file\n";
        return 0;
    }
    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cout << "could not open file\n";
        return 0;
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(file), {});
    file.close();

    bool disasCompatible = argc == 3;

    if (!disasCompatible)
        std::cout << "{\n";
    int i = 0;
    for (auto c : buf) {
        std::cout << (!disasCompatible ? "0x" : "") << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(static_cast<unsigned char>(c)) << (!disasCompatible ? ", " : " ");
        ++i;
        if (i % 4 == 0) {
            std::cout << "\n";
        }
    }

    if (!disasCompatible)
        std::cout << "}";
    std::cout << std::endl;

    std::cout << "\n\nBytes: " << std::dec << buf.size() << std::endl;

    return 0;
}