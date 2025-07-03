#ifndef  INTERVAL_SPLITTER
#define INTERVAL_SPLITTER

#include <pair.h>

namespace paresy_s {

    class IntervalSplitter {
    public:
        class Iterator {
        public:

            Iterator() = default;

            Iterator(int current, int end, int chunk)
                : current_(current), end_(end), chunk_size_(chunk) {
            }

            Pair<int> operator*() const {
                int to = current_ + chunk_size_;
                if (to > end_) to = end_;
                return { current_, to };
            }

            Iterator& operator++() {
                current_ += chunk_size_;
                if (current_ > end_) current_ = end_;
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator& other) const {
                return current_ == other.current_;
            }

            bool operator!=(const Iterator& other) const {
                return !(*this == other);
            }

        private:
            int current_ = 0;
            int end_ = 0;
            int chunk_size_ = 1;
        };

        IntervalSplitter(int start, int end, int chunk)
            : start_(start), end_(end), chunk_size_(chunk) {
        }

        Iterator begin() const {
            return Iterator(start_, end_, chunk_size_);
        }

        Iterator end() const {
            return Iterator(end_, end_, chunk_size_);
        }

    private:
        int start_;
        int end_;
        int chunk_size_;
    };

    inline IntervalSplitter splitInterval(int start, int end, int chunk_size) {
        return IntervalSplitter(start, end, chunk_size);
    }

}

#endif //INTERVAL_SPLITTER