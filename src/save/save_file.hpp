#ifndef SAVE_FILE_HPP
#define SAVE_FILE_HPP

#include <cstdint>
#include <fstream>

namespace gbaemu::save
{

    class SaveFile
    {
      private:
        std::fstream file;
        bool isNewFile;
        uint32_t size;

        void extractSaveFileSize();

      public:
        SaveFile(const char *path, bool &success, uint32_t fallBackSize);
        ~SaveFile();

        bool isNewSaveFile() const
        {
            return isNewFile;
        }
        uint32_t getSize() const
        {
            return size;
        }
        void eraseAll()
        {
            erase(0, size);
        }
        void expandSaveFileSize(uint32_t newSize);
        void erase(uint32_t offset, uint32_t size);
        void fill(uint32_t offset, uint32_t size, char value);
        void read(uint32_t offset, char *readBuf, uint32_t size);
        void write(uint32_t offset, const char *writeBuf, uint32_t size);
    };

} // namespace gbaemu::save
#endif