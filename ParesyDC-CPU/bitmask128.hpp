#ifndef BITMASK128_HPP
#define BITMASK128_HPP

#include <cstdint>
#include <iostream>
#include <iomanip>

#ifdef __CUDACC__
    #define HD __host__ __device__
#else
    #define HD
#endif

struct alignas(16) bitmask128 {
    uint64_t low;
    uint64_t high;

    // Constructors
    HD bitmask128() : low(0), high(0) {}
    HD bitmask128(uint64_t h, uint64_t l) : low(l), high(h) {}
    HD bitmask128(int value) : low(static_cast<uint64_t>(value)), high(0) {}
    HD bitmask128(uint64_t value) : low(value), high(0) {}

    // Bitwise OR
    HD inline bitmask128 operator|(const bitmask128& other) const {
        return bitmask128(high | other.high, low | other.low);
    }

    // Bitwise AND
    HD inline bitmask128 operator&(const bitmask128& other) const {
        return bitmask128(high & other.high, low & other.low);
    }

    // Bitwise OR assignment
    HD bitmask128& operator|=(const bitmask128& other) {
        high |= other.high;
        low |= other.low;
        return *this;
    }

    // Bitwise AND assignment
    HD bitmask128& operator&=(const bitmask128& other) {
        high &= other.high;
        low &= other.low;
        return *this;
    }

    // Bitwise NOT
    HD inline bitmask128 operator~() const {
        return bitmask128(~high, ~low);
    }

    // Left shift
    HD inline bitmask128 operator<<(int shift) const {
        if (shift == 0) return *this;
        if (shift >= 128) return bitmask128(0, 0);
        if (shift >= 64) return bitmask128(low << (shift - 64), 0);
        uint64_t newHigh = (high << shift) | (low >> (64 - shift));
        uint64_t newLow = low << shift;
        return bitmask128(newHigh, newLow);
    }

    // Right shift
    HD inline bitmask128 operator>>(int shift) const {
        if (shift == 0) return *this;
        if (shift >= 128) return bitmask128(0, 0);
        if (shift >= 64) return bitmask128(0, high >> (shift - 64));
        uint64_t newLow = (low >> shift) | (high << (64 - shift));
        uint64_t newHigh = high >> shift;
        return bitmask128(newHigh, newLow);
    }

    // Comparison
    HD bool operator==(const bitmask128& other) const {
        return high == other.high && low == other.low;
    }

    HD bool operator!=(const bitmask128& other) const {
        return high != other.high || low != other.low;
    }

    HD inline operator bool() const {
        return high != 0 || low != 0;
    }

    // Output
    friend inline std::ostream& operator<<(std::ostream& os, const bitmask128& mask) {
        /*for (int i = 63; i >= 0; --i) {
            os << ((mask.high >> i) & 1);
        }
        for (int i = 63; i >= 0; --i) {
            os << ((mask.low >> i) & 1);
        }
        return os;*/

        os << std::hex << "0x" << mask.high << std::setfill('0') << std::setw(16) << mask.low << std::dec;
        return os;
    }
};

#endif // BITMASK128_HPP