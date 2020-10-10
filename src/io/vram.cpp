#include "vram.hpp"
#include "lcd/defs.hpp"
#include "memory_defs.hpp"
#include "util.hpp"

#include <algorithm>

namespace gbaemu
{
    VRAM::VRAM()
    {
        vram = new uint8_t[memory::VRAM_LIMIT - memory::VRAM_OFFSET + 1];
    }

    VRAM::~VRAM()
    {
        delete[] vram;
        vram = nullptr;
    }

    void VRAM::reset()
    {
        std::fill_n(vram, memory::VRAM_LIMIT - memory::VRAM_OFFSET + 1, 0);
    }

    uint32_t VRAM::handleMirroring(uint32_t addr)
    {
        // Even though VRAM is sized 96K (64K+32K),
        // it is repeated in steps of 128K
        // (64K+32K+32K, the two 32K blocks itself being mirrors of each other)

        // First handle 128K mirroring!
        addr &= memory::VRAM_OFFSET | ((static_cast<uint32_t>(128) << 10) - 1);

        // Now lets handle upper 32K mirroring! (subtract 32K if >= 96K (last 32K block))
        addr -= (addr >= (memory::VRAM_OFFSET | (static_cast<uint32_t>(96) << 10)) ? (static_cast<uint32_t>(32) << 10) : 0);

        return addr;
    }

    uint8_t VRAM::read8(uint32_t addr) const
    {
        addr = handleMirroring(addr) - memory::VRAM_OFFSET;
        return vram[addr];
    }
    uint16_t VRAM::read16(uint32_t addr) const
    {
        addr = (handleMirroring(addr) & ~1) - memory::VRAM_OFFSET;
        return le(*reinterpret_cast<uint16_t *>(vram + addr));
    }
    uint32_t VRAM::read32(uint32_t addr) const
    {
        addr = (handleMirroring(addr) & ~3) - memory::VRAM_OFFSET;
        return le(*reinterpret_cast<uint32_t *>(vram + addr));
    }

    void VRAM::write8(uint32_t addr, uint8_t value)
    {
        addr = handleMirroring(addr) & ~1;

        //TODO is this legit?
        bool bitMapMode = (vram[0] & lcd::DISPCTL::BG_MODE_MASK) >= 4;
        // Consists of BG & PBJ
        // In bitmap mode:
        // 0x06014000-0x06017FFF ignored
        // 0x06000000-0x0613FFFF as BG_OBJ_RAM

        // Not in bitmap mode:
        // 0x06010000-0x06017FFF ignored
        // 0x06000000-0x0600FFFF as BG_OBJ_RAM

        // Writes to BG (6000000h-600FFFFh) (or 6000000h-6013FFFh in Bitmap mode)
        // and to Palette (5000000h-50003FFh) are writing
        //the new 8bit value to BOTH upper and lower 8bits of the addressed halfword,
        // ie. "[addr AND NOT 1]=data*101h".
        if (addr <= static_cast<uint32_t>(bitMapMode ? 0x0613FFFF : 0x0600FFFF)) {
            addr -= memory::VRAM_OFFSET;
            // As both bytes are the same we do not need an le() call
            *reinterpret_cast<uint16_t *>(vram + addr) = (static_cast<uint16_t>(value) << 8) | value;
        }
        // Else ignored
    }

    void VRAM::write16(uint32_t addr, uint16_t value)
    {
        addr = (handleMirroring(addr) & ~1) - memory::VRAM_OFFSET;
        *reinterpret_cast<uint16_t *>(vram + addr) = le(value);
    }
    void VRAM::write32(uint32_t addr, uint32_t value)
    {
        addr = (handleMirroring(addr) & ~3) - memory::VRAM_OFFSET;
        *reinterpret_cast<uint32_t *>(vram + addr) = le(value);
    }
} // namespace gbaemu
