#pragma once

#include <cstdint>

namespace ElfBug
{
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;

    // x64 only (for now)
    typedef uint64 ptr;

#define ElfBugArchValue(x32value, x64value) (x64value)
}
