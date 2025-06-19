#ifndef BITMASK128_HPP
#define BITMASK128_HPP

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <functional>

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
    HD explicit bitmask128(size_t value) : low(value), high(0) {}

    // Bitwise OR
    HD inline bitmask128 operator|(const bitmask128& other) const {
        return bitmask128(high | other.high, low | other.low);
    }

    HD inline bitmask128 operator|(const size_t& other) const {
        return bitmask128(high, low | other);
    }

    // Bitwise AND
    HD inline bitmask128 operator&(const bitmask128& other) const {
        return bitmask128(high & other.high, low & other.low);
    }

    HD inline bitmask128 operator&(const size_t& other) const {
        return bitmask128(high, low & other);
    }

    // Bitwise OR assignment
    HD bitmask128& operator|=(const bitmask128& other) {
        high |= other.high;
        low |= other.low;
        return *this;
    }

    HD bitmask128& operator|=(const size_t& other) {
        low |= other;
        return *this;
    }

    // Bitwise AND assignment
    HD bitmask128& operator&=(const bitmask128& other) {
        high &= other.high;
        low &= other.low;
        return *this;
    }

    HD bitmask128& operator&=(const size_t& other) {
        low &= other;
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

    HD bitmask128& operator<<=(int shift) {
        if (shift == 0) {
            return *this;
        }
        else if (shift >= 128) {
            high = 0;
            low = 0;
        }
        else if (shift >= 64) {
            high = low << (shift - 64);
            low = 0;
        }
        else {
            high = (high << shift) | (low >> (64 - shift));
            low <<= shift;
        }
        return *this;
    }

    HD bitmask128& operator>>=(int shift) {
        if (shift == 0) {
            return *this;
        }
        else if (shift >= 128) {
            high = 0;
            low = 0;
        }
        else if (shift >= 64) {
            low = high >> (shift - 64);
            high = 0;
        }
        else {
            low = (low >> shift) | (high << (64 - shift));
            high >>= shift;
        }
        return *this;
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

namespace std {
    template <>
    struct hash<bitmask128> {
        std::size_t operator()(const bitmask128& s) const {
            std::size_t h1 = std::hash<uint64_t>{}(s.low);
            std::size_t h2 = std::hash<uint64_t>{}(s.high);
            return h1 ^ (h2 << 1);
        }
    };
}

#endif // BITMASK128_HPP