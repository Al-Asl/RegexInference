#ifndef BITMASK_H
#define BITMASK_H

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <functional>
#include <pair.h>

template <class T>
using Pair = paresy_s::Pair<T>;

#define HD __host__ __device__

namespace paresy_s
{
    template <int N>
    struct bitmask {
        HD bitmask(const uint64_t(&input)[N]) {
            for (size_t i = 0; i < N; ++i) {
                data[i] = input[i];
            }
        }
        HD bitmask() {
            for (size_t i = 0; i < N; ++i) {
                data[i] = 0;
            }
        }

        HD static bitmask all() {
            uint64_t vals[N];
            for (size_t i = 0; i < N; ++i) {
                vals[i] = (uint64_t)-1;
            }
            return bitmask(vals);
        }
        HD static bitmask one() {
            uint64_t vals[N] = {};
            vals[0] = 1;
            return bitmask(vals);
        }

        HD Pair<uint64_t> get128Hash() const {

            if (N == 2) 
            { return { data[1], data[0] }; }

            uint64_t lCS = 0, hCS = 0;
#if RELAX_UNIQUENESS_CHECK_TYPE == 0
            const int stride = (N * 64) / 126;

            int j = 0;
            for (int i = 0; i < N; ++i) {
                for (int k = 0; k < 64; k += stride, ++j) {
                    if (j < 63) {
                        if (data[i] & ((uint64_t)1 << k)) lCS |= (uint64_t)1 << j;
                    }
                    else if (j < 126) {
                        if (data[i] & ((uint64_t)1 << k)) hCS |= (uint64_t)1 << (j - 63);
                    }
                    else break;
                }
            }
#elif RELAX_UNIQUENESS_CHECK_TYPE == 1
            int j = 0;
            for (int i = 0; i < N; ++i) {
                uint64_t bitPtr = 1;
                int maxbitsForThisTrace = (126 * 64 + 64 * N) / 64 * N;
                for (int k = 0; k < maxbitsForThisTrace; ++k, ++j, bitPtr <<= 1) {
                    if (j < 63) {
                        if (data[i] & bitPtr) lCS |= (uint64_t)1 << j;
                    }
                    else if (j < 126) {
                        if (data[i] & bitPtr) hCS |= (uint64_t)1 << (j - 63);
                    }
                    else break;
                }
            }
#elif RELAX_UNIQUENESS_CHECK_TYPE == 2
            for (int i = 0; i < N; ++i) {
                uint64_t x = data[i];
                x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
                x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
                x = x ^ (x >> 31);
                if (i < N / 2) hCS ^= x; else lCS ^= x;
            }
#else
            int j = 0;
            for (int i = 0; i < N; ++i) {
                uint64_t bitPtr = 1;
                for (int k = 0; k < 64; ++k, ++j, bitPtr <<= 1) {
                    if (j < 63) {
                        if (data[i] & bitPtr) lCS |= (uint64_t)1 << j;
                    }
                    else if (j < 126) {
                        if (data[i] & bitPtr) hCS |= (uint64_t)1 << (j - 63);
                    }
                    else break;
                }
            }
#endif

            return { hCS, lCS };
        }

        HD bitmask operator|(const bitmask& other) const {
            uint64_t vals[N];
            for (size_t i = 0; i < N; ++i) {
                vals[i] = data[i] | other.data[i];
            }
            return bitmask(vals);
        }

        HD bitmask& operator|=(const bitmask& other) {
            for (size_t i = 0; i < N; ++i) {
                data[i] |= other.data[i];
            }
            return *this;
        }

        HD bitmask operator&(const bitmask& other) const {
            uint64_t vals[N];
            for (size_t i = 0; i < N; ++i) {
                vals[i] = data[i] & other.data[i];
            }
            return bitmask(vals);
        }

        HD bitmask& operator&=(const bitmask& other) {
            for (size_t i = 0; i < N; ++i) {
                data[i] &= other.data[i];
            }
            return *this;
        }

        HD bitmask operator<<(const int shift) const {
            uint64_t vals[N] = {};

            if (shift < 0) {
                return *this >> (-shift);
            }

            const int bits = 64;
            int wordShift = shift / bits;
            int bitShift = shift % bits;

            for (size_t i = 0; i < N; ++i) {
                if (i + wordShift >= N) break;

                vals[i + wordShift] |= data[i] << bitShift;

                if (bitShift != 0 && (i + wordShift + 1) < N) {
                    vals[i + wordShift + 1] |= data[i] >> (bits - bitShift);
                }
            }

            return bitmask(vals);
        }
        HD bitmask& operator<<=(int shift) {

            if (shift < 0) {
                return *this >>= -shift;
            }

            const int bits = 64;
            int wordShift = shift / bits;
            int bitShift = shift % bits;

            if (wordShift >= N) {
                for (int i = 0; i < N; ++i) data[i] = 0;
                return *this;
            }

            for (int i = N - 1; i >= wordShift; --i) {
                data[i] = data[i - wordShift];
            }
            for (int i = wordShift - 1; i >= 0; --i) {
                data[i] = 0;
            }

            if (bitShift > 0) {
                for (int i = N - 1; i > 0; --i) {
                    data[i] <<= bitShift;
                    data[i] |= data[i - 1] >> (bits - bitShift);
                }
                data[0] <<= bitShift;
            }

            return *this;
        }

        HD bitmask operator>>(const int shift) const {

            uint64_t vals[N] = {};

            if (shift < 0) {
                return *this << (-shift);
            }

            const int bits = 64;
            int wordShift = shift / bits;
            int bitShift = shift % bits;

            for (size_t i = 0; i < N; ++i) {
                if (i < wordShift) continue;

                int j = i - wordShift;
                vals[j] |= data[i] >> bitShift;

                if (bitShift != 0 && j > 0) {
                    vals[j - 1] |= data[i] << (bits - bitShift);
                }
            }

            return bitmask(vals);
        }
        HD bitmask& operator>>=(int shift) {

            if (shift < 0) {
                return *this <<= -shift;
            }

            const int bits = 64;
            int wordShift = shift / bits;
            int bitShift = shift % bits;

            if (wordShift >= N) {
                for (int i = 0; i < N; ++i) { data[i] = 0; }
                return *this;
            }

            for (int i = 0; i < N - wordShift; ++i) {
                data[i] = data[i + wordShift];
            }
            for (int i = N - wordShift; i < N; ++i) {
                data[i] = 0;
            }

            if (bitShift > 0) {
                for (int i = 0; i < N - 1; ++i) {
                    data[i] >>= bitShift;
                    data[i] |= data[i + 1] << (bits - bitShift);
                }
                data[N - 1] >>= bitShift;
            }

            return *this;
        }

        HD bitmask operator~() const {
            uint64_t vals[N];
            for (size_t i = 0; i < N; ++i) {
                vals[i] = ~data[i];
            }
            return bitmask(vals);
        }

        HD bool operator==(const bitmask& other) const {
            for (size_t i = 0; i < N; ++i) {
                if (data[i] != other.data[i]) { return false; }
            }
            return true;
        }
        HD bool operator!=(const bitmask& other) const {
            for (size_t i = 0; i < N; ++i) {
                if (data[i] != other.data[i]) { return true; }
            }
            return false;
        }

        HD inline operator bool() const {
            for (size_t i = 0; i < N; ++i) {
                if (data[i] != 0) { return true; }
            }
            return false;
        }

        HD inline bool operator!() const {
            return !static_cast<bool>(*this);
        }

        friend std::ostream& operator<<(std::ostream& os, const bitmask& mask) {
            os << std::hex << "0x";
            for (int i = N - 1; i >= 0; --i) {
                os << std::setfill('0') << std::setw(16) << mask.data[i];
            }
            os << std::dec;
            return os;
        }

        void print() {
            printf("from high to low:");
            for (int i = N - 1; i >= 0; --i) {
                if (i == 0) {
                    printf(" %llu\n", data[i]);
                }
                else {
                    printf(" %llu,", data[i]);
                }
            }
        }

    private:
        uint64_t data[N];
    };
}

namespace std {
    template <int N>
    struct hash<paresy_s::bitmask<N>> {
        HD std::size_t operator()(const paresy_s::bitmask<N>& s) const {
            auto [high, low] = s.get128Hash();
            std::size_t h1 = std::hash<uint64_t>{}(low);
            std::size_t h2 = std::hash<uint64_t>{}(high);
            return h1 ^ (h2 << 1);
        }
    };
}

#endif //BITMASK_H