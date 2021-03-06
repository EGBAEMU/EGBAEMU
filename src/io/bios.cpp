#include "bios.hpp"

#include "memory.hpp"
#include "util.hpp"

#include <algorithm>

namespace gbaemu
{
    const uint8_t Bios::fallbackBios[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x0a, 0x00, 0x00, 0xea,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xea,
        0x00, 0x00, 0x00, 0x00,
        0x0f, 0x50, 0x2d, 0xe9,
        0x01, 0x03, 0xa0, 0xe3,
        0x00, 0xe0, 0x8f, 0xe2,
        0x04, 0xf0, 0x10, 0xe5,
        0x0f, 0x50, 0xbd, 0xe8,
        0x04, 0xf0, 0x5e, 0xe2,
        0x00, 0x58, 0x2d, 0xe9,
        0x02, 0xc0, 0x5e, 0xe5,
        0x00, 0xb0, 0x4f, 0xe1,
        0x04, 0xb0, 0x2d, 0xe5,
        0x80, 0xb0, 0x0b, 0xe2,
        0x1f, 0xb0, 0x8b, 0xe3,
        0x0b, 0xf0, 0x21, 0xe1,
        0x04, 0xe0, 0x2d, 0xe5,
        0x74, 0xe0, 0xa0, 0xe3,
        0x0b, 0x00, 0x5c, 0xe3,
        0xf8, 0xb0, 0xa0, 0x03,
        0x01, 0x00, 0x00, 0x0a,
        0x0c, 0x00, 0x5c, 0xe3,
        0x94, 0xb0, 0xa0, 0x03,
        0x1b, 0xff, 0x2f, 0x01,
        0x04, 0xe0, 0x9d, 0xe4,
        0x93, 0xf0, 0x21, 0xe3,
        0x04, 0xc0, 0x9d, 0xe4,
        0x0c, 0xf0, 0x69, 0xe1,
        0x00, 0x58, 0xbd, 0xe8,
        0x0e, 0xf0, 0xb0, 0xe1,
        0x00, 0x00, 0x00, 0x00,
        0x04, 0x20, 0xa0, 0xe3,
        0xf8, 0x47, 0x2d, 0xe9,
        0x02, 0x36, 0xa0, 0xe3,
        0x01, 0x30, 0x43, 0xe2,
        0x03, 0x00, 0x12, 0xe1,
        0x12, 0x00, 0x00, 0x0a,
        0x01, 0x04, 0x12, 0xe3,
        0x03, 0x20, 0x02, 0xe0,
        0x04, 0x00, 0x00, 0x1a,
        0xf8, 0x07, 0xb0, 0xe8,
        0xf8, 0x07, 0xa1, 0xe8,
        0x08, 0x20, 0x52, 0xe2,
        0x08, 0x00, 0x00, 0xca,
        0x0a, 0x00, 0x00, 0xea,
        0x00, 0x30, 0x90, 0xe5,
        0x03, 0x40, 0xa0, 0xe1,
        0x03, 0x50, 0xa0, 0xe1,
        0x03, 0x60, 0xa0, 0xe1,
        0x03, 0x70, 0xa0, 0xe1,
        0x03, 0x80, 0xa0, 0xe1,
        0x03, 0x90, 0xa0, 0xe1,
        0x03, 0xa0, 0xa0, 0xe1,
        0xf8, 0x07, 0xa1, 0xe8,
        0x08, 0x20, 0x52, 0xe2,
        0xfc, 0xff, 0xff, 0xca,
        0xf8, 0x87, 0xbd, 0xe8,
        0x0f, 0x40, 0x2d, 0xe9,
        0x02, 0x36, 0xa0, 0xe3,
        0x01, 0x30, 0x43, 0xe2,
        0x03, 0x00, 0x12, 0xe1,
        0x1a, 0x00, 0x00, 0x0a,
        0x01, 0x04, 0x12, 0xe3,
        0x0c, 0x00, 0x00, 0x1a,
        0x01, 0x03, 0x12, 0xe3,
        0x03, 0x20, 0x02, 0xe0,
        0x04, 0x00, 0x00, 0x1a,
        0xb2, 0x30, 0xd0, 0xe0,
        0xb2, 0x30, 0xc1, 0xe0,
        0x01, 0x20, 0x52, 0xe2,
        0xfb, 0xff, 0xff, 0xca,
        0x10, 0x00, 0x00, 0xea,
        0x04, 0x30, 0x90, 0xe4,
        0x04, 0x30, 0x81, 0xe4,
        0x01, 0x20, 0x52, 0xe2,
        0xfb, 0xff, 0xff, 0xca,
        0x0b, 0x00, 0x00, 0xea,
        0x01, 0x03, 0x12, 0xe3,
        0x03, 0x20, 0x02, 0xe0,
        0xb0, 0x30, 0xd0, 0x11,
        0x00, 0x30, 0x90, 0x05,
        0x03, 0x00, 0x00, 0x1a,
        0xb2, 0x30, 0xc1, 0xe0,
        0x01, 0x20, 0x52, 0xe2,
        0xfc, 0xff, 0xff, 0xca,
        0x02, 0x00, 0x00, 0xea,
        0x04, 0x30, 0x81, 0xe4,
        0x01, 0x20, 0x52, 0xe2,
        0xfc, 0xff, 0xff, 0xca,
        0x0f, 0x80, 0xbd, 0xe8};

    void Bios::setExternalBios(const uint8_t *bios, size_t biosSize)
    {
        if (externalBios) {
            delete[] this->bios;
        }
        externalBios = true;
        this->biosSize = biosSize;
        uint8_t *newBios = new uint8_t[biosSize];
        std::copy_n(bios, biosSize, newBios);
        this->bios = newBios;
    }

    Bios::Bios() : bios(fallbackBios), biosSize(sizeof(fallbackBios) / sizeof(fallbackBios[0])), externalBios(false), biosState(BIOS_AFTER_STARTUP)
    {
    }

    uint8_t Bios::read8(uint32_t addr) const
    {
        if (execInBios && addr < biosSize) {
            return bios[addr];
        } else {
            return biosState >> ((addr & 3) << 3);
        }
    }

    uint16_t Bios::read16(uint32_t addr) const
    {
        uint32_t alignedAddr = addr & ~1;
        if (execInBios && alignedAddr + sizeof(uint16_t) - 1 < biosSize) {
            return le(*reinterpret_cast<const uint16_t *>(bios + alignedAddr));
        } else {
            return biosState >> ((addr & 2) << 3);
        }
    }
    uint16_t Bios::read16Inst(uint32_t addr)
    {
        execInBios = true;

        uint32_t alignedAddr = addr & ~1;
        if (alignedAddr + sizeof(uint16_t) - 1 < biosSize) {
            uint16_t data = le(*reinterpret_cast<const uint16_t *>(bios + alignedAddr));
            biosState = data;
            return data;
        } else {
            return biosState >> ((addr & 2) << 3);
        }
    }

    uint32_t Bios::read32(uint32_t addr) const
    {
        uint32_t alignedAddr = addr & ~3;
        if (execInBios && alignedAddr + sizeof(uint32_t) - 1 < biosSize) {
            return le(*reinterpret_cast<const uint32_t *>(bios + alignedAddr));
        } else {
            return biosState;
        }
    }
    uint32_t Bios::read32Inst(uint32_t addr)
    {
        execInBios = true;

        uint32_t alignedAddr = addr & ~3;
        if (alignedAddr + sizeof(uint32_t) - 1 < biosSize) {
            uint32_t data = le(*reinterpret_cast<const uint32_t *>(bios + alignedAddr));
            biosState = data;
            return data;
        } else {
            return biosState;
        }
    }
} // namespace gbaemu