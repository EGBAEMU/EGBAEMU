#include "io_regs.hpp"
namespace gbaemu
{
    bool operator<(const IO_Mapped &a, const IO_Mapped &b)
    {
        return a.upperBound < b.lowerBound;
    }

    bool operator<(const IO_Mapped &a, const uint32_t b)
    {
        return a.upperBound < b;
    }

    bool operator<(const uint32_t a, const IO_Mapped &b)
    {
        return a < b.lowerBound;
    }
} // namespace gbaemu