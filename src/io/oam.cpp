#include "oam.hpp"
#include "memory_defs.hpp"
#include "util.hpp"

#include <algorithm>

namespace gbaemu
{

#define GBA_ALLOC_MEM_REG(x) new uint8_t[x##_LIMIT - x##_OFFSET + 1]
#define GBA_MEM_CLEAR(arr, x) std::fill_n(arr, x##_LIMIT - x##_OFFSET + 1, 0)
#define GBA_MEM_CLEAR_VALUE(arr, x, value) std::fill_n(arr, x##_LIMIT - x##_OFFSET + 1, (value))

    OAM::OAM()
    {
        mem = GBA_ALLOC_MEM_REG(memory::OAM);
    }

    OAM::~OAM()
    {
        delete[] mem;
    }

    void OAM::reset()
    {
        GBA_MEM_CLEAR(mem, memory::OAM);
        for (auto &obj : objects) {
            obj.enabled = false;
        }
    }

    void OAM::delegateDecode(uint32_t offset, uint16_t value) {
        uint8_t innerOffset = offset & 7;
        // OBJ Entries are mapped on 3 * uint16_t at every multiple of 0x8
        if (offset < 0x6) {
            uint8_t objOffset = offset / 8;
            objects[objOffset].writeAndDecode16(offset, value);
        }
    }

    void OAM::write16(uint32_t offset, uint16_t value)
    {
        *reinterpret_cast<uint16_t *>(mem + offset) = le(value);
        delegateDecode(offset, value);
    }
    void OAM::write32(uint32_t offset, uint32_t value)
    {
        *reinterpret_cast<uint32_t *>(mem + offset) = le(value);

        // need to split the data in half
        delegateDecode(offset, value);
        delegateDecode(offset + 2, value >> 16);
    }

} // namespace gbaemu