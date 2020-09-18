#include "save_file.hpp"

namespace gbaemu::save
{

    SaveFile::SaveFile(const char *path, bool &success, uint32_t size) : size(size)
    {
        bool isNewFile = !std::ifstream(path).good();

        if (isNewFile) {
            std::ofstream out(path, std::ios::binary);
            out.close();
        }

        file.open(path, std::ios::binary | std::ios::in | std::ios::out);
        success = file.is_open();

        if (success && isNewFile) {
            erase(0, size);
        }
    }

    SaveFile::~SaveFile()
    {
        if (file.is_open())
            file.close();
    }

    void SaveFile::fill(uint32_t offset, uint32_t size, char value)
    {
        // seek to offset
        file.seekp(file.beg + offset);

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
    }

    void SaveFile::erase(uint32_t offset, uint32_t size)
    {
        fill(offset, size, static_cast<char>(0xFF));
    }

    void SaveFile::read(uint32_t offset, char *readBuf, uint32_t size)
    {
        file.seekg(file.beg + offset);
        file.read(readBuf, size);
    }

    void SaveFile::write(uint32_t offset, const char *writeBuf, uint32_t size)
    {
        file.seekp(file.beg + offset);
        file.write(writeBuf, size);
    }

} // namespace gbaemu::save
