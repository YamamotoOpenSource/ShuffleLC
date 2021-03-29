
#ifndef INCLUDED_SHUFFLELC_H
#define INCLUDED_SHUFFLELC_H

#include <stdint.h>

#include <array>

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
} // namespace ShuffleLinearCongruentInner

class ShuffleLinearCongruent {
public:
    ShuffleLinearCongruent(uint32_t seed = 12345);
    ~ShuffleLinearCongruent();
    uint64_t get();
    operator uint64_t() { return get(); }

private:
    std::array<ShuffleLinearCongruentInner::bitState, 64> bitState;
    uint8_t thrTbl[256];
    uint64_t lastCarry = 0;
};

#endif
