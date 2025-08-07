#include <rei.h>

#include <map>
#include <set>
#include <tuple>
#include <chrono>

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <device_functions.h>

#include <thrust/copy.h>
#include <thrust/remove.h>
#include <thrust/device_ptr.h>
#include <warpcore/hash_set.cuh>

#include <pair.h>
#include <interval_splitter.h>
#include <bitmask.h>

template <class T>
using Pair = paresy_s::Pair<T>;

#if CS_BIT_COUNT == 0
using CS = paresy_s::bitmask<2>;
#elif CS_BIT_COUNT == 1
using CS = paresy_s::bitmask<4>;
#elif CS_BIT_COUNT == 2
using CS = paresy_s::bitmask<8>;
#elif CS_BIT_COUNT == 3
using CS = paresy_s::bitmask<16>;
#elif CS_BIT_COUNT == 4
using CS = paresy_s::bitmask<32>;
#else
using CS = paresy_s::bitmask<64>;
#endif


#define HD __host__ __device__

#if LOG_LEVEL == 3
#define LOG_OP(context, cost, op_string, dif) \
        int tbc = dif; \
        if (tbc) printf("Cost %-2d | (%s) | AllREs: %-11llu | StoredREs: %-10d | ToBeChecked: %-10d \n", \
            cost, op_string.c_str() ,context.allREs, context.lastIdx, tbc);
#else
#define LOG_OP(context, cost, op_string, dif)
#endif

// ============= Cuda helpers =============

inline
cudaError_t checkCuda(cudaError_t res) {
    if (res != cudaSuccess) {
        fprintf(stderr, "CUDA Runtime Error: %s\n", cudaGetErrorString(res));
        assert(res == cudaSuccess);
    }
    return res;
}

void checkAllCudaErrors() {
    cudaError_t err = cudaGetLastError();
    while (err != cudaSuccess) checkCuda(err);
}


size_t getFreeMemory() {
    size_t free_mem = 0, total_mem = 0;
    cudaError_t err = cudaMemGetInfo(&free_mem, &total_mem);
    return free_mem;
}

// ============= guide table =============

// Shortlex ordering
struct strComparison {
    bool operator () (const std::string& str1, const std::string& str2) const {
        if (str1.length() == str2.length()) return str1 < str2;
        return str1.length() < str2.length();
    }
};

// Generating the infix of a string
std::set<std::string, strComparison> infixesOf(const std::string& word) {
    std::set<std::string, strComparison> ic;
    for (int len = 0; len <= word.length(); ++len) {
        for (int index = 0; index < word.length() - len + 1; ++index) {
            ic.insert(word.substr(index, len));
        }
    }
    return ic;
}

std::set<std::string, strComparison> generatingIC(const std::vector<std::string>& pos, const std::vector<std::string>& neg) {
    // Generating infix-closure (ic) of the input strings
    std::set<std::string, strComparison> ic = {};

    for (const std::string& word : pos) {
        std::set<std::string, strComparison> set1 = infixesOf(word);
        ic.insert(set1.begin(), set1.end());
    }
    for (const std::string& word : neg) {
        std::set<std::string, strComparison> set1 = infixesOf(word);
        ic.insert(set1.begin(), set1.end());
    }
    return ic;
}

// constant memory needs to be global, only 64kb in size
__constant__ uint64_t deviceData[64 * 128];

class GuideTable {
public:

    class Iterator {
        const CS* ptr;
    public:
        HD Iterator(const CS* p) : ptr(p) {}

        HD Pair<CS> operator*() const { return Pair<CS>(*ptr, *(ptr + 1)); }
        HD Iterator& operator++() { ptr += 2; return *this; }
        HD bool operator!=(const Iterator& other) const { return *ptr; }
    };

    class RowIterator {
    public:
        HD RowIterator(const CS* data, int gtColumns, int rowIndex) : row(rowIndex), data(data), gtColumns(gtColumns) {}
        HD Iterator begin() {
            return Iterator(data + row * gtColumns);
        }
        HD Iterator end() {
            return Iterator(data + (row + 1) * gtColumns);
        }
    private:
        const CS* data;
        int row;
        int gtColumns;
    };

    class Device {
    public:

        Device(int ICsize, int gtColumns, int alphabetSize, CS* d_Data) : ICsize(ICsize), gtColumns(gtColumns), alphabetSize(alphabetSize), d_Data(d_Data){ }

        __device__ RowIterator IterateRow(int rowIndex) const {
#ifdef GUIDE_TABLE_CONSTANT_MEMORY
            return RowIterator(reinterpret_cast<CS*>(deviceData), gtColumns, rowIndex);
#else
            return RowIterator(d_Data, gtColumns, rowIndex);
#endif
        }

        int ICsize;
        int gtColumns;
        int alphabetSize;
        const CS* d_Data;
    };

    Device deviceTable() {
        return Device(ICsize, gtColumns, alphabetSize, d_Data);
    }

    GuideTable(std::vector<std::vector<CS>> gt, int alphabetSize) : alphabetSize(alphabetSize)
    {
        ICsize = static_cast<int> (gt.size());
        gtColumns = static_cast<int> (gt.back().size());

        data = new CS[ICsize * gtColumns];

        for (int i = 0; i < ICsize; ++i) {
            for (int j = 0; j < gt.at(i).size(); ++j) {
                data[i * gtColumns + j] = gt.at(i).at(j);
            }
        }
        copyToDevice();
    }

    GuideTable() : ICsize(0), gtColumns(0), alphabetSize(0), data(nullptr) {}

    ~GuideTable() {
        if (data != nullptr) {
            delete[] data;
            freeDevice();
        }
    }

    RowIterator IterateRow(int rowIndex) const {
        return RowIterator(data, gtColumns, rowIndex);
    }

    int ICsize;
    int gtColumns;
    int alphabetSize;

private:
    CS* data;
    CS* d_Data;

    void copyToDevice() {

        int bytes = ICsize * gtColumns * sizeof(CS);

#ifdef GUIDE_TABLE_CONSTANT_MEMORY
        checkCuda(cudaMemcpyToSymbol(deviceData, data, bytes));
#else
        checkCuda(cudaMalloc(&d_Data, bytes));
        checkCuda(cudaMemcpy(d_Data, data, bytes, cudaMemcpyHostToDevice));
#endif
    }

    void freeDevice() {

#ifdef GUIDE_TABLE_CONSTANT_MEMORY
        uint64_t zeros[64 * 128] = { 0 }; // constant memory is only 64kb
        cudaMemcpyToSymbol(deviceData, zeros, sizeof(zeros));
#else
        checkCuda(cudaFree(d_Data));
#endif
    }
};

bool generatingGuideTable(GuideTable* guideTable, const std::set<std::string, strComparison>& ic)
{
    int alphabetSize = -1;
    for (auto& word : ic) {
        if (word.size() > 1) break;
        alphabetSize++;
    }

    std::vector<std::vector<CS>> gt;

    for (auto& word : ic) {
        std::vector<CS> row;
        for (int i = 1; i < word.length(); ++i) {

            int index1 = 0;
            for (auto& w : ic) {
                if (w == word.substr(0, i)) break;
                index1++;
            }
            int index2 = 0;
            for (auto& w : ic) {
                if (w == word.substr(i)) break;
                index2++;
            }

            row.push_back(CS::one() << index1);
            row.push_back(CS::one() << index2);
        }

        row.push_back(CS());
        gt.push_back(row);
    }

#ifdef GUIDE_TABLE_CONSTANT_MEMORY
    int tableSize = static_cast<int> (gt.back().size()) * static_cast<int> (gt.size());
    if (tableSize * sizeof(CS) > 64 * 1024)
    {
#if LOG_LEVEL >= 2
        printf("Your input needs a guide table of size %u bytes which can't fit in constant memory.\n", tableSize * sizeof(CS));
#endif
        return false;
    }
#endif

    if (gt.size() > sizeof(CS) * 8) {
#if LOG_LEVEL >= 2
        printf("Your input needs %lu bits which exceeds %lu bits ", gt.size(), sizeof(CS) * 8);
        printf("(current version).\nPlease use less/shorter words and run the code again.\n");
#endif
        return false;
    }

    new (guideTable) GuideTable(gt, alphabetSize);
    return true;
}

// Generating of the guide table only once for the whole enumeration process
bool generatingGuideTable(GuideTable& guideTable, CS& posBits, CS& negBits,
    const std::vector<std::string>& pos, const std::vector<std::string>& neg) {

    std::set<std::string, strComparison> ic = generatingIC(pos, neg);

    if (!generatingGuideTable(&guideTable, ic))
        return false;

    for (auto& p : pos) {
        int wordIndex = distance(ic.begin(), ic.find(p));
        posBits |= (CS::one() << wordIndex);
    }

    for (auto& n : neg) {
        int wordIndex = distance(ic.begin(), ic.find(n));
        negBits |= (CS::one() << wordIndex);
    }

    return true;
}

// ============= operations =============

enum class Opreation { Question = 0, Star = 1, Concatenate = 2, Or = 3, Count = 4 };

std::string to_string(Opreation op) {
    switch (op)
    {
    case Opreation::Question:
        return "Q";
    case Opreation::Star:
        return "S";
    case Opreation::Concatenate:
        return "C";
    case Opreation::Or:
        return "O";
    default:
        break;
    }
    return "";
}

__device__ inline CS processQuestion(const CS& cs) {
    return cs | CS::one();
}

__device__ inline CS processStar(const GuideTable::Device& guideTable, const CS& cs) {

    auto cs1 = cs | CS::one();
    int ix = guideTable.alphabetSize + 1;
    CS c = CS::one() << ix;

    while (ix < guideTable.ICsize) 
    {
        if (!(cs1 & c)) {
            for (auto [left, right] : guideTable.IterateRow(ix)) {
                if ((left & cs1) && (right & cs1)) { cs1 |= c; break; }
            }
        }
        c <<= 1; ix++;
    }

    return cs1;
}

__device__ inline Pair<CS> processConcatenate(const GuideTable::Device& guideTable,const CS& left, const CS& right) {

    CS cs1 = CS();
    if (left & CS::one()) cs1 |= right;
    if (right & CS::one()) cs1 |= left;
    CS cs2 = cs1;

    int ix = guideTable.alphabetSize + 1;
    CS c = CS::one() << ix;

    while (ix < guideTable.ICsize)
    {
        // when CS have value that means one of parts contains phi, check above
        if (!(cs1 & c)) {
            for (auto [l, r] : guideTable.IterateRow(ix))
                if ((l & left) && (r & right)) { cs1 |= c; break; }
                }

        if (!(cs2 & c)) {
            for (auto [l, r] : guideTable.IterateRow(ix))
                if ((l & right) && (r & left)) { cs2 |= c; break; }
        }

        c <<= 1; ix++;
    }

    return { cs1, cs2 };
}

__device__ inline CS processOr(const CS& left, const CS& right) {
    return left | right;
}

__device__ inline CS processAnd(const CS& left, const  CS& right) {
    return left & right;
}

// ============= Context =============

class CostIntervals {
public:
    CostIntervals(const unsigned short maxCost) {
        opCount = static_cast<int>(Opreation::Count);
        startPoints = new int[(maxCost + 2) * opCount]();
    }
    ~CostIntervals() {
        delete[] startPoints;
    }
    std::tuple<int, int> Interval(int cost, Opreation start = Opreation::Question, Opreation end = Opreation::Or) const {
        return std::make_tuple(this->start(cost, start), this->end(cost, end));
    }
    int& start(int cost, Opreation op) {
        return startPoints[cost * opCount + static_cast<int>(op)];
    }
    int start(int cost, Opreation op) const {
        return startPoints[cost * opCount + static_cast<int>(op)];
    }
    int& end(int cost, Opreation op) {
        return startPoints[cost * opCount + static_cast<int>(op) + 1];
    }
    int end(int cost, Opreation op) const {
        return startPoints[cost * opCount + static_cast<int>(op) + 1];
    }
    void indexToCost(int index, int& cost, Opreation& op) const {
        int i = 0;
        while (index >= startPoints[i]) { i++; }
        i--;
        cost = i / opCount;
        op = static_cast<Opreation>(i % opCount);
    }
private:
    int* startPoints;
    int opCount;
};

struct DeviceHashSet
{
    using hash_set_t = warpcore::HashSet<
        uint64_t,         // key type
        uint64_t(0) - 1,  // empty key
        uint64_t(0) - 2,  // tombstone key
        warpcore::probing_schemes::QuadraticProbing<warpcore::hashers::MurmurHash <uint64_t>>>;

    __host__ DeviceHashSet(int capacity) : cHashSet(capacity), iHashSet(capacity) {}

    inline __device__ bool insert(uint64_t high, uint64_t low) {
        return insert(high, low, warpcore::cg::tiled_partition<1>(warpcore::cg::this_thread_block()));
    }

    inline __device__ bool insert(uint64_t high, uint64_t low, const warpcore::cg::thread_block_tile<1>& group) {
        int H = cHashSet.insert(high, group);
        int L = cHashSet.insert(low, group);
        H = (H > 0) ? H : -H;
        L = (L > 0) ? L : -L;
        uint64_t HL = H; HL <<= 32; HL |= L;
        return !(iHashSet.insert(HL, group) > 0);
    }

private:
    hash_set_t cHashSet;
    hash_set_t iHashSet;
};

// Initialising the hashSets with empty, epsilon and alphabet before starting the enumeration
__global__ void hashSetsInitialisation(DeviceHashSet d_visited, CS* d_langCache, int alphabetSize)
{
    // Adding empty to the hashSet
    d_visited.insert(0,0);

    // Adding eps to the hashSet
    d_visited.insert(0,1);

    // Adding alphabet to the hashSet
    for (int i = 0; i < alphabetSize; ++i) {
        auto [high, low] = d_langCache[i].get128Hash();
        d_visited.insert(high, low);
    }
}

class Context {
public:

    static Pair<uint64_t> getCacheCapacity(uint64_t memory_size, double tempRatio = 0.5) {

        uint64_t cacheCapacity = memory_size / (
            (sizeof(CS) + sizeof(int) * 2 + sizeof(uint64_t) * 4) +
            (sizeof(CS) + sizeof(int) * 2) * tempRatio);

        return { cacheCapacity , (uint64_t)(cacheCapacity * tempRatio) };
    }

    Context(int cache_capacity, int temp_cache_capacity, CS posBits, CS negBits)
        : cache_capacity(cache_capacity), temp_cache_capacity(temp_cache_capacity), posBits(posBits), negBits(negBits),
        d_visited(cache_capacity * 2) {

        FinalREIdx = new int[1]; *FinalREIdx = -1;

        checkCuda(cudaMalloc(&d_FinalREIdx, sizeof(int)));
        checkCuda(cudaMemcpy(d_FinalREIdx, FinalREIdx, sizeof(int), cudaMemcpyHostToDevice));

        checkCuda(cudaMalloc(&d_langCache, cache_capacity * sizeof(CS)));
        checkCuda(cudaMalloc(&d_temp_langCache, temp_cache_capacity * sizeof(CS)));
        checkCuda(cudaMalloc(&d_leftIdx, cache_capacity * sizeof(int)));
        checkCuda(cudaMalloc(&d_rightIdx, cache_capacity * sizeof(int)));
        checkCuda(cudaMalloc(&d_temp_leftIdx, temp_cache_capacity * sizeof(int)));
        checkCuda(cudaMalloc(&d_temp_rightIdx, temp_cache_capacity * sizeof(int)));

        lastIdx = 0;
        isFound = false;
        allREs = 0;
        onTheFly = false;
    }

    ~Context() {

        delete[] FinalREIdx;

        checkCuda(cudaFree(d_FinalREIdx));
        checkCuda(cudaFree(d_langCache));
        checkCuda(cudaFree(d_temp_langCache));
        checkCuda(cudaFree(d_leftIdx));
        checkCuda(cudaFree(d_rightIdx));
        checkCuda(cudaFree(d_temp_leftIdx));
        checkCuda(cudaFree(d_temp_rightIdx));
    }

    class Device 
    {
    public:
        Device(Context& context)
            : d_langCache(context.d_langCache),
            d_temp_langCache(context.d_temp_langCache),
            d_visited(context.d_visited),
            d_temp_leftIdx(context.d_temp_leftIdx),
            d_temp_rightIdx(context.d_temp_rightIdx),
            d_FinalREIdx(context.d_FinalREIdx),
            onTheFly(context.onTheFly),
            posBits(context.posBits),
            negBits(context.negBits)
        {
        }

        const CS* d_langCache;
        CS* d_temp_langCache;
        DeviceHashSet d_visited;
        int* d_temp_leftIdx;
        int* d_temp_rightIdx;
        int* d_FinalREIdx;

        bool onTheFly;
        CS posBits, negBits;

        __device__ inline void insert(const CS& CS, int tid, int ldx, int rdx = 0) {
            insert(CS, tid, ldx, rdx, warpcore::cg::tiled_partition<1>(warpcore::cg::this_thread_block()));
        }

        __device__ inline void insert(const CS& CS, int tid, int ldx, int rdx, const warpcore::cg::thread_block_tile<1>& group) {

            if (onTheFly) {

                if ((CS & posBits) == posBits && (~CS & negBits) == negBits) {
                    *d_FinalREIdx = tid;
                    d_temp_langCache[tid] = CS;
                    d_temp_leftIdx[tid] = ldx;
                    d_temp_rightIdx[tid] = rdx;
                }
            }
            else {
                auto [high, low] = CS.get128Hash();
                if (d_visited.insert(high, low)) {
                    d_temp_langCache[tid] = CS;
                    d_temp_leftIdx[tid] = ldx;
                    d_temp_rightIdx[tid] = rdx;
                    if ((CS & posBits) == posBits && (~CS & negBits) == negBits)
                        atomicCAS(d_FinalREIdx, -1, tid);
                }
                else {
                    d_temp_langCache[tid] = CS::all();
                    d_temp_leftIdx[tid] = -1;
                    d_temp_rightIdx[tid] = -1;
                }
            }
        }
    };

    Device deviceContext() {
        return Device(*this);
    }

    // Checking empty, epsilon, and the alphabet
    bool intialCheck(int alphaCost, const std::vector<std::string>& pos, const std::vector<std::string>& neg, std::string& RE)
    {
        // Initialisation of the alphabet
        for (auto& word : pos) for (auto ch : word) alphabet.insert(ch);
        for (auto& word : neg) for (auto ch : word) alphabet.insert(ch);

        LOG_OP((*this), alphaCost, std::string("A"), static_cast<int>(alphabet.size()) + 2)

        // Checking empty
        allREs++;
        if (pos.empty()) { RE = "Empty"; return true; }

        // Checking epsilon
        allREs++;
        if ((pos.size() == 1) && (pos.at(0).empty())) { RE = "eps"; return true; }

        // Checking the alphabet
        CS idx = CS::one() << 1; // Pointing to the position of the first char of the alphabet (idx 1 is for epsilon)
        auto* langCache = new CS[alphabet.size()];
        auto alphabetSize = static_cast<int> (alphabet.size());

        for (int i = 0; lastIdx < alphabetSize; ++i) {

            langCache[i] = idx;

            allREs++;

            std::string s(1, *next(alphabet.begin(), i));
            if ((pos.size() == 1) && (pos.at(0) == s)) { RE = s; return true; }

            idx <<= 1;
            lastIdx++;
        }

        checkCuda(cudaMemcpy(d_langCache, langCache, alphabetSize * sizeof(CS), cudaMemcpyHostToDevice));
        hashSetsInitialisation<<<1, 1>>>(d_visited, d_langCache, alphabetSize);

        delete[] langCache;

        return false;
    }

    bool syncAndCheck(int REs) {
        checkCuda(cudaMemcpy(FinalREIdx, d_FinalREIdx, sizeof(int), cudaMemcpyDeviceToHost));
        allREs += REs;
        if (*FinalREIdx != -1) { isFound = true; return true; }
        if (!onTheFly) storeUniqueREs(REs);
        return false;
    }

    void printLangCahce() {
        auto cache = new CS[lastIdx];
        checkCuda(cudaMemcpy(cache, d_langCache, sizeof(CS) * lastIdx, cudaMemcpyDeviceToHost));
        for (size_t i = 0; i < lastIdx; i++) { cache[i].print(); }
        delete[] cache;
    }

    void storeUniqueREs(int N) {
        thrust::device_ptr<CS> new_end_ptr;
        thrust::device_ptr<CS> d_langCache_ptr(d_langCache + lastIdx);
        thrust::device_ptr<CS> d_temp_langCache_ptr(d_temp_langCache);
        thrust::device_ptr<int> d_leftIdx_ptr(d_leftIdx + lastIdx);
        thrust::device_ptr<int> d_rightIdx_ptr(d_rightIdx + lastIdx);
        thrust::device_ptr<int> d_temp_leftIdx_ptr(d_temp_leftIdx);
        thrust::device_ptr<int> d_temp_rightIdx_ptr(d_temp_rightIdx);

        new_end_ptr = // end of d_temp_langCache
            thrust::remove(d_temp_langCache_ptr, d_temp_langCache_ptr + N, CS::all());
        thrust::remove(d_temp_leftIdx_ptr, d_temp_leftIdx_ptr + N, -1);
        thrust::remove(d_temp_rightIdx_ptr, d_temp_rightIdx_ptr + N, -1);

        // It stores all (or a part of) unique CSs until language cahce gets full
        // If language cache gets full, it makes onTheFly mode on
        int numberOfNewUniqueREs = static_cast<int>(new_end_ptr - d_temp_langCache_ptr);
        if (lastIdx + numberOfNewUniqueREs > cache_capacity) {
            N = cache_capacity - lastIdx;
            onTheFly = true;
#if LOG_LEVEL >= 2
            printf("==== switch to \"OnTheFly\" ====\n");
#endif
        }
        else N = numberOfNewUniqueREs;

        thrust::copy_n(d_temp_langCache_ptr, N, d_langCache_ptr);
        thrust::copy_n(d_temp_leftIdx_ptr, N, d_leftIdx_ptr);
        thrust::copy_n(d_temp_rightIdx_ptr, N, d_rightIdx_ptr);

        lastIdx += N;
    }

    std::set<char> alphabet;

    int cache_capacity;
    int temp_cache_capacity;

    CS* d_langCache;
    CS* d_temp_langCache;
    DeviceHashSet d_visited;
    int* d_leftIdx;
    int* d_rightIdx;
    int* d_temp_leftIdx;
    int* d_temp_rightIdx;
    int* d_FinalREIdx;

    uint64_t allREs;
    // Index of the last free position in the language cache
    uint64_t lastIdx;
    bool isFound;
    bool onTheFly;
    int* FinalREIdx;
    CS posBits, negBits;
};

__global__ void QuestionMark(Pair<int> interval, Context::Device context)
{
    const int tid = blockDim.x * blockIdx.x + threadIdx.x;

    if (tid < (interval.right - interval.left)) {

        auto CS = context.d_langCache[(interval.left + tid)];

        CS = processQuestion(CS);

        context.insert(CS, tid, interval.left + tid);
    }
}

__global__ void Star(Pair<int> interval, GuideTable::Device guideTable, Context::Device context)
{
    const int tid = blockDim.x * blockIdx.x + threadIdx.x;

    if (tid < (interval.right - interval.left)) {

        auto CS = context.d_langCache[(interval.left + tid)];

        CS = processStar(guideTable, CS);

        context.insert(CS, tid, interval.left + tid);
    }
}

__global__ void Concat(Pair<int> lInterval, Pair<int> rInterval, GuideTable::Device guideTable, Context::Device context)
{
    const int tid = blockDim.x * blockIdx.x + threadIdx.x;

    if (tid < (lInterval.right - lInterval.left) * (rInterval.right - rInterval.left)) {

        const auto group = warpcore::cg::tiled_partition<1>(warpcore::cg::this_thread_block());

        int ldx = lInterval.left + tid / (rInterval.right - rInterval.left);
        CS lCS = context.d_langCache[ldx];

        int rdx = rInterval.left + tid % (rInterval.right - rInterval.left);
        CS rCS = context.d_langCache[rdx];

        auto [lr, rl] = processConcatenate(guideTable, lCS, rCS);

        context.insert(lr, tid * 2, ldx, rdx, group);
        context.insert(rl, tid * 2 + 1, rdx, ldx, group);
    }
}

__global__ void OrEpsilon(Pair<int> interval, Context::Device context)
{
    const int tid = blockDim.x * blockIdx.x + threadIdx.x;

        if (tid < (interval.right - interval.left)) {

            auto CS = context.d_langCache[(interval.left + tid)];

            CS = processOr(CS, CS::one());

            context.insert(CS, tid, interval.left + tid);
        }
}

__global__ void Or(Pair<int> lInterval, Pair<int> rInterval, Context::Device context)
{
    const int tid = blockDim.x * blockIdx.x + threadIdx.x;

    if (tid < (lInterval.right - lInterval.left) * (rInterval.right - rInterval.left)) {

        int ldx = lInterval.left + tid / (rInterval.right - rInterval.left);
        CS lCS = context.d_langCache[ldx];

        int rdx = rInterval.left + tid % (rInterval.right - rInterval.left);
        CS rCS = context.d_langCache[rdx];

        auto CS = processOr(lCS, rCS);

        context.insert(CS, tid, ldx, rdx);
    }
}

// ============= To String =============

// Finding the left and right indices that makes the final RE to bring to the host later
__global__ void generateResIndices(
    const int index,
    const int alphabetSize,
    const int* d_leftIdx,
    const int* d_rightIdx,
    int* d_FinalREIdx)
{

    int resIdx = 0;
    while (d_FinalREIdx[resIdx] != -1) resIdx++;
    int queue[600];
    queue[0] = index;
    int head = 0;
    int tail = 1;
    while (head < tail) {
        int re = queue[head];
        int l = d_leftIdx[re];
        int r = d_rightIdx[re];
        d_FinalREIdx[resIdx++] = re;
        d_FinalREIdx[resIdx++] = l;
        d_FinalREIdx[resIdx++] = r;
        if (l >= alphabetSize) queue[tail++] = l;
        if (r >= alphabetSize) queue[tail++] = r;
        head++;
    }

}

// Adding parentheses if needed
std::string bracket(std::string s) {
    int p = 0;
    for (int i = 0; i < s.length(); i++) {
        if (s[i] == '(') p++;
        else if (s[i] == ')') p--;
        else if (s[i] == '+' && p <= 0) return "(" + s + ")";
    }
    return s;
}

// Generating the final RE string recursively
// When all the left and right indices are ready in the host
std::string toString(
    int index,
    std::map<int, std::pair<int, int>>& indicesMap,
    const std::set<char>& alphabet,
    const CostIntervals& intervals)
{

    if (index == -2) return "eps"; // Epsilon
    if (index == -1) return "Error";
    if (index < alphabet.size()) {
        std::string s(1, *next(alphabet.begin(), index));
        return s;
    }

    int cost; Opreation op;
    intervals.indexToCost(index, cost, op);

    if (op == Opreation::Question) {
        std::string res = toString(indicesMap[index].first, indicesMap, alphabet, intervals);
        if (res.length() > 1) return "(" + res + ")?";
        return res + "?";
    }

    if (op == Opreation::Star) {
        std::string res = toString(indicesMap[index].first, indicesMap, alphabet, intervals);
        if (res.length() > 1) return "(" + res + ")*";
        return res + "*";
    }

    if (op == Opreation::Concatenate) {
        std::string left = toString(indicesMap[index].first, indicesMap, alphabet, intervals);
        std::string right = toString(indicesMap[index].second, indicesMap, alphabet, intervals);
        return bracket(left) + bracket(right);
    }

    std::string left = toString(indicesMap[index].first, indicesMap, alphabet, intervals);
    std::string right = toString(indicesMap[index].second, indicesMap, alphabet, intervals);
    return left + "+" + right;

}

// Bringing the left and right indices of the RE from device to host
std::string REtoString(const Context& context,const CostIntervals& intervals)
{
    auto* LIdx = new int[1];
    auto* RIdx = new int[1];

    checkCuda(cudaMemcpy(LIdx, context.d_temp_leftIdx + *context.FinalREIdx, sizeof(int), cudaMemcpyDeviceToHost));
    checkCuda(cudaMemcpy(RIdx, context.d_temp_rightIdx + *context.FinalREIdx, sizeof(int), cudaMemcpyDeviceToHost));

    auto alphabetSize = static_cast<int> (context.alphabet.size());

    int* d_resIndices;
    checkCuda(cudaMalloc(&d_resIndices, 600 * sizeof(int)));

    thrust::device_ptr<int> d_resIndices_ptr(d_resIndices);
    thrust::fill(d_resIndices_ptr, d_resIndices_ptr + 600, -1);

    if (*LIdx >= alphabetSize) generateResIndices <<<1, 1 >>> (*LIdx, alphabetSize, context.d_leftIdx, context.d_rightIdx, d_resIndices);
    if (*RIdx >= alphabetSize) generateResIndices <<<1, 1 >>> (*RIdx, alphabetSize, context.d_leftIdx, context.d_rightIdx, d_resIndices);

    int resIndices[600];
    checkCuda(cudaMemcpy(resIndices, d_resIndices, 600 * sizeof(int), cudaMemcpyDeviceToHost));

    std::map<int, std::pair<int, int>> indicesMap;

    indicesMap.insert(std::make_pair(INT_MAX - 1, std::make_pair(*LIdx, *RIdx)));

    int i = 0;
    while (resIndices[i] != -1 && i + 2 < 600) {
        int re = resIndices[i];
        int l = resIndices[i + 1];
        int r = resIndices[i + 2];
        indicesMap.insert(std::make_pair(re, std::make_pair(l, r)));
        i += 3;
    }

    if (i + 2 >= 600) return "Size of the output is too big";

    cudaFree(d_resIndices);

    return toString(INT_MAX - 1, indicesMap, context.alphabet, intervals);
}

// ============= REI =============

struct Costs
{
    int alpha;
    int question;
    int star;
    int concat;
    int alternation; //or
    Costs(const unsigned short* costFun) {
        alpha = costFun[0];
        question = costFun[1];
        star = costFun[2];
        concat = costFun[3];
        alternation = costFun[4];
    }
};

bool checkTime(std::chrono::steady_clock::time_point startTime, double maxTime) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count();
    return duration >= maxTime;
}

paresy_s::Result paresy_s::REI(const unsigned short* costFun, const unsigned short maxCost, const std::vector<std::string>& pos, const std::vector<std::string>& neg, double maxTime) {

    auto startTime = std::chrono::steady_clock::now();

    Costs costs(costFun);

    GuideTable guideTable;
    CS posBits{}, negBits{};
    if (!generatingGuideTable(guideTable, posBits, negBits, pos, neg))
    { return paresy_s::Result("not_found", 0, 0, 0); }

    uint64_t available_memory = (getFreeMemory() * 4) / 5; // 80% for the free memory
    auto [ langCacheCapacity, temp_langCacheCapacity] = Context::getCacheCapacity(available_memory);

#if LOG_LEVEL >= 2
    printf("The amount of memory that will be allocated: %lf mb.\n", available_memory / ((double)1024 * 1024));
    printf("The max amount of RE that will be stored: %lu. The ICSize is %u\n", langCacheCapacity, guideTable.ICsize);
#endif

    Context context(langCacheCapacity, temp_langCacheCapacity, posBits, negBits);
    CostIntervals intervals(maxCost);

    std::string RE;

    if (context.intialCheck(costs.alpha, pos, neg, RE)) return paresy_s::Result(RE, 0, context.allREs, guideTable.ICsize);

    intervals.end(costs.alpha, Opreation::Concatenate) = context.lastIdx;
    intervals.end(costs.alpha, Opreation::Or) = context.lastIdx;

    int thread_count = 128;

    int shortageCost = -1; bool lastRound = false;
    bool useQuestionOverOr = costs.alpha + costs.alternation >= costs.question;

    int cost{};

    for (cost = costs.alpha + 1; cost <= maxCost; ++cost) {

        // Once it uses a previous cost that is not fully stored, it should continue as the last round
        if (context.onTheFly) {
            int dif = cost - shortageCost;
            if (dif == costs.question || dif == costs.star || dif == costs.alpha + costs.concat || dif == costs.alpha + costs.alternation) lastRound = true;
        }
        
        // Question mark
        if (cost >= costs.alpha + costs.question && useQuestionOverOr) {
            // ignore results from (*) and (?)
            auto [start, end] = intervals.Interval(cost - costs.question, static_cast<Opreation>(2));
            for (auto interval : paresy_s::splitInterval(start, end, temp_langCacheCapacity))
            {
                int N = (interval.right - interval.left);
                LOG_OP(context, cost, to_string(Opreation::Question), N)
                int qBlc = (N + thread_count - 1) / thread_count;
                QuestionMark<<<qBlc, thread_count>>> (interval, context.deviceContext());
                checkCuda(cudaGetLastError());
                if (context.syncAndCheck(N)) {
                    intervals.end(cost, Opreation::Question) = INT_MAX; goto exitEnumeration;
                }

                if (checkTime(startTime, maxTime)) { goto exitEnumeration; }
            }
        }
        intervals.end(cost, Opreation::Question) = context.lastIdx;

        // Star
        if (cost >= costs.alpha + costs.star) {
            // ignore results from (*) and (?)
            auto [start, end] = intervals.Interval(cost - costs.star, static_cast<Opreation>(2));
            for (auto interval : paresy_s::splitInterval(start, end, temp_langCacheCapacity))
            {
                int N = (interval.right - interval.left);
                LOG_OP(context, cost, to_string(Opreation::Star), N)
                    int qBlc = (N + thread_count - 1) / thread_count;
                Star<<<qBlc, thread_count>>>(interval, guideTable.deviceTable(), context.deviceContext());
                checkCuda(cudaGetLastError());
                if (context.syncAndCheck(N)) {
                    intervals.end(cost, Opreation::Star) = INT_MAX; goto exitEnumeration;
                }

                if (checkTime(startTime, maxTime)) { goto exitEnumeration; }
            }
        }
        intervals.end(cost, Opreation::Star) = context.lastIdx;

        
        //Concat
        for (int i = costs.alpha; 2 * i <= cost - costs.concat; ++i) {

            auto [lstart, lend] = intervals.Interval(i);
            auto [rstart, rend] = intervals.Interval(cost - i - costs.concat);

            for (auto interval : paresy_s::splitInterval(rstart, rend, temp_langCacheCapacity/(2*(lend - lstart))))
            {
                int N = (interval.right - interval.left) * (lend - lstart);
                LOG_OP(context, cost, to_string(Opreation::Concatenate), 2 * N)

                int qBlc = (N + thread_count - 1) / thread_count;
                Concat<<<qBlc, thread_count >>>(interval, Pair<int>(lstart, lend), guideTable.deviceTable(), context.deviceContext());
                checkCuda(cudaGetLastError());
                if (context.syncAndCheck(N * 2)) {
                    intervals.end(cost, Opreation::Concatenate) = INT_MAX; goto exitEnumeration;
                }

                if (checkTime(startTime, maxTime)) { goto exitEnumeration; }
            }
        }
        intervals.end(cost, Opreation::Concatenate) = context.lastIdx;

        //Or
        if (!useQuestionOverOr && cost >= 2 * costs.alpha + costs.alternation) {

            auto [start, end] = intervals.Interval(cost - costs.alpha - costs.alternation);

            for (auto interval : paresy_s::splitInterval(start, end, temp_langCacheCapacity))
            {
                int N = (interval.right - interval.left);
                LOG_OP(context, cost, to_string(Opreation::Or), N)
                int qBlc = (N + thread_count - 1) / thread_count;
                OrEpsilon<<<qBlc, thread_count >>>(interval, context.deviceContext());
                checkCuda(cudaGetLastError());
                if (context.syncAndCheck(N)) {
                    intervals.end(cost, Opreation::Or) = INT_MAX; goto exitEnumeration;
                }

                if (checkTime(startTime, maxTime)) { goto exitEnumeration; }
            }
        }
        for (int i = costs.alpha; 2 * i <= cost - costs.alternation; ++i) {

            auto [lstart, lend] = intervals.Interval(i);
            auto [rstart, rend] = intervals.Interval(cost - i - costs.alternation);

            for (auto interval : paresy_s::splitInterval(rstart, rend, temp_langCacheCapacity / (lend - lstart)))
            {
                int N = (interval.right - interval.left) * (lend - lstart);
                LOG_OP(context, cost, to_string(Opreation::Or), N)

                int qBlc = (N + thread_count - 1) / thread_count;
                Or<<<qBlc, thread_count >>>(interval, Pair<int>(lstart, lend), context.deviceContext());
                checkCuda(cudaGetLastError());
                if (context.syncAndCheck(N)) {
                    intervals.end(cost, Opreation::Or) = INT_MAX; goto exitEnumeration;
                }

                if (checkTime(startTime, maxTime)) { goto exitEnumeration; }
            }
        }
        intervals.end(cost, Opreation::Or) = context.lastIdx;

        if (lastRound) break;
        if (context.onTheFly && shortageCost == -1) shortageCost = cost;
    }

    exitEnumeration:

    if (context.isFound)
    {
#if LOG_LEVEL >= 2
        if(context.onTheFly){ printf("\"OnTheFly\" mode has been used\n"); }
#endif
        RE = REtoString(context, intervals);
        return Result(RE, cost, context.allREs, guideTable.ICsize);
    }

#if LOG_LEVEL >= 2
    if (checkTime(startTime, maxTime))
    { printf("exceeded the time limit %lf\n", maxTime); }
    else if (cost > maxCost)
    { printf("Max cost exceeded!\n"); }
    else 
    { printf("memory limit exceeded, %lf mb of memory has been used.\n", available_memory/((double)1024*1024)); }
#endif

    return paresy_s::Result("not_found", cost > maxCost ? maxCost : cost, context.allREs, guideTable.ICsize);
}