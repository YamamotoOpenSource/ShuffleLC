
#include <printf.h>

#include "ShuffleLC.h"

namespace {
const struct {
    uint32_t bias;
    uint32_t shuffle;
} mixTbl[] = {{3000000037, 3000000077}, {3000000109, 3000000209},
              {3000000253, 3000000257}, {3000000277, 3000000349},
              {3000000361, 3000000397}, {3000000401, 3000000433},
              {3000000457, 3000000461}, {3000000473, 3000000517},
              {3000000593, 3000000673}, {3000000713, 3000000749},
              {3000000781, 3000000821}, {3000000833, 3000000889},
              {3000000973, 3000001049}, {3000001057, 3000001093},
              {3000001153, 3000001177}, {3000001229, 3000001321},
              {3000001337, 3000001513}, {3000001549, 3000001601},
              {3000001621, 3000001633}, {3000001661, 3000001709},
              {3000001721, 3000001801}, {3000001817, 3000001897},
              {3000001957, 3000001993}, {3000001997, 3000002077},
              {3000002141, 3000002149}, {3000002197, 3000002209},
              {3000002273, 3000002297}, {3000002333, 3000002417},
              {3000002437, 3000002497}, {3000002501, 3000002521},
              {3000002561, 3000002569}, {3000002633, 3000002657},
              {3000002669, 3000002693}, {3000002729, 3000002741},
              {3000002813, 3000002849}, {3000002893, 3000002981},
              {3000002989, 3000003037}, {3000003089, 3000003113},
              {3000003133, 3000003217}, {3000003301, 3000003313},
              {3000003337, 3000003341}, {3000003353, 3000003389},
              {3000003437, 3000003493}, {3000003521, 3000003529},
              {3000003533, 3000003557}, {3000003569, 3000003649},
              {3000003653, 3000003673}, {3000003733, 3000003757},
              {3000003761, 3000003817}, {3000003829, 3000003893},
              {3000003913, 3000003953}, {3000003961, 3000004009},
              {3000004037, 3000004081}, {3000004121, 3000004193},
              {3000004201, 3000004237}, {3000004409, 3000004489},
              {3000004537, 3000004549}, {3000004573, 3000004577},
              {3000004589, 3000004597}, {3000004621, 3000004649},
              {3000004769, 3000004849}, {3000004901, 3000004933},
              {3000004957, 3000004993}, {3000005017, 3000005053}};
const uint32_t baseCoef = 3000000037;
} // namespace

namespace ShuffleLinearCongruentInner {
class CarryData {
public:
    CarryData(uint64_t d) : data(d) {}
    operator uint64_t() const { return data; }
    CarryData &operator=(uint64_t d) {
        data = d;
        return *this;
    }
    void swap() {
        uint32_t t = s[0];
        s[0] = s[1];
        s[1] = t;
    }

private:
    union {
        uint64_t data;
        struct {
            uint32_t s[2];
        };
    };
};

int bitState::idx() const {
    return oct[0] ^ oct[3];
}
int bitState::mix() const {
    return oct[1] ^ oct[2];
}

template <typename T> void bitState::update(CarryData &carry, const T &coef) {
    data = carry =
        carry + static_cast<uint64_t>(data) * coef.shuffle + coef.bias;
    carry.swap();
}

void bitState::update(CarryData &carry) {
    data = carry = carry + data * baseCoef + baseCoef;
    carry.swap();
}

} // namespace ShuffleLinearCongruentInner

ShuffleLinearCongruent::ShuffleLinearCongruent(uint32_t seed) {
    uint64_t d = baseCoef + baseCoef * seed;
    for(int i = 0; i < 256; ++i) {
        thrTbl[i] = i + seed;
    }
    for(int i = 0; i < 64; ++i) {
        bitState[i].setData(d);
        d += baseCoef;
    }
}

ShuffleLinearCongruent::~ShuffleLinearCongruent() {}
uint64_t ShuffleLinearCongruent::get() {
    uint64_t data = 0;
    uint64_t bit = 1;
    ShuffleLinearCongruentInner::CarryData carry{lastCarry};
    for(int i = 0; i < 64; ++i) {
        if(1) {
            bitState[i].update(carry, mixTbl[i]);
        } else {
            bitState[i].update(carry);
        }
        auto idx = bitState[i].idx();
        auto mix = bitState[i].mix();
        thrTbl[idx] += mix;

        if(thrTbl[idx] & (1 << (carry & 7))) {
            data |= bit;
        }
        bit <<= 1;
    }

    lastCarry = carry;

    return data;
}
