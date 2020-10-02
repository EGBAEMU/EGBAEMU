#include "save_file.hpp"

#include <iostream>

namespace gbaemu::save
{

    SaveFile::SaveFile(const char *path, bool &success, uint32_t fallBackSize) : size(fallBackSize)
    {
        isNewFile = !std::ifstream(path).good();

        if (isNewFile) {
            std::ofstream out(path, std::ios::binary);
            out.close();
        }

        file.open(path, std::ios::binary | std::ios::in | std::ios::out);
        success = file.is_open();

        if (!isNewFile) {
            extractSaveFileSize();
        } else if (success && isNewFile) {
            eraseAll();
        }
    }

    SaveFile::~SaveFile()
    {
        if (file.is_open()) {
            file.flush();
            file.close();
        }
    }

    void SaveFile::extractSaveFileSize()
    {
        file.seekg(0, file.end);
        size = file.tellg();
    }

    void SaveFile::expandSaveFileSize(uint32_t newSize)
    {
        if (newSize > size) {
            erase(size, newSize - size);
            size = newSize;
            isNewFile = false;
        } else if (newSize < size) {
            size = newSize;
            std::cout << "WARNING: save file shrinking not supported!" << std::endl;
        } else {
            isNewFile = false;
        }
    }

    void SaveFile::fill(uint32_t offset, uint32_t size, char value)
    {
        // seek to offset
        file.seekp(offset, file.beg);

        // Fill with needed size!
        const auto prevWidth = file.width();
        const auto prevFill = file.fill();
        // Set fill character
        file.fill(value);
        // Set target file size
        file.width(size);
        // Write single fill char -> rest will be filled automagically
        file << value;

        // Restore default settings
        file.fill(prevFill);
        file.width(prevWidth);

        file.flush();
    }

    void SaveFile::erase(uint32_t offset, uint32_t size)
    {
        fill(offset, size, static_cast<char>(0xFF));
    }

    void SaveFile::read(uint32_t offset, char *readBuf, uint32_t size)
    {
        file.seekg(offset, file.beg);
        file.read(readBuf, size);
    }

    void SaveFile::write(uint32_t offset, const char *writeBuf, uint32_t size)
    {
        file.seekp(offset, file.beg);
        file.write(writeBuf, size);
        file.flush();
    }

} // namespace gbaemu::save
