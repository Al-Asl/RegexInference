#ifndef PAIR_H
#define PAIR_H

#define HD __host__ __device__

namespace paresy_s {

    // pair of two element that share the same type. Can be used on both host and device
    template<typename T>
    struct Pair {
        T left;
        T right;

        HD inline Pair(T left, T right)
            : left(left), right(right) {
        }
    };

}

#endif // Pair


