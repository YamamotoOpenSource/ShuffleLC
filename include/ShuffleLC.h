
#ifndef INCLUDED_SHUFFLELC_H
#define INCLUDED_SHUFFLELC_H

#include <stdint.h>

#include <array>
#include <initializer_list>
#include <memory>
#include <vector>

namespace ShuffleLinearCongruentInner {
class CarryData;
class bitState {
public:
    bitState(int32_t seed = 1) : data(seed) {}
    ~bitState() = default;

    int idx() const;
    int mix() const;
    template <typename T> void update(CarryData &carry, const T &coef);
    void update(CarryData &carry);

    uint32_t getData() const { return data; }
    void setData(uint32_t dat) { data = dat; }

private:
    union {
        struct {
            uint8_t oct[4];
        };
        uint32_t data;
    };
};

class shuffleBase;

} // namespace ShuffleLinearCongruentInner

class ShuffleLinearCongruent {
public:
    enum class SHUFFLE {
        PRIVATE, // 独立
        PROCESS, // Processで共有
        SYSTEM   // Systemで共有
    };

public:
    ShuffleLinearCongruent(uint32_t seed = 12345,
                           SHUFFLE type = SHUFFLE::PRIVATE);
    ShuffleLinearCongruent(SHUFFLE type = SHUFFLE::PRIVATE,
                           uint32_t seed = 12345)
        : ShuffleLinearCongruent(seed, type) {}
    ShuffleLinearCongruent(const std::initializer_list<uint32_t> &seed,
                           SHUFFLE type = SHUFFLE::PRIVATE);
    ShuffleLinearCongruent(const std::initializer_list<uint32_t> &seed,
                           const std::initializer_list<uint8_t> &shuffle,
                           SHUFFLE type = SHUFFLE::PRIVATE);
    virtual ~ShuffleLinearCongruent();
    uint64_t get();
    operator uint64_t() { return get(); }

private:
    void setState(const std::initializer_list<uint32_t> &seed);

private:
    std::array<ShuffleLinearCongruentInner::bitState, 64> bitState;
    // uint8_t thrTbl[256];
    std::unique_ptr<ShuffleLinearCongruentInner::shuffleBase> shuffle;
    uint64_t lastCarry = 0;
};

#endif
