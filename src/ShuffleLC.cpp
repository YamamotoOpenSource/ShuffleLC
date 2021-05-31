
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ShuffleLC.h"

#define SHARE_MEMORY_NAME "/SHUFFLE_SHARE_MEMORY"

namespace {
// ビットの0/1の確率が半分の適当なテーブル
// シャッフルテーブルを直接参照するとシャッフルテーブルの状態によって
// ビット割当の偏りが出るので、常に1/2になるように配分テーブルから取得する
const uint8_t BitRndTbl[256] = {
    0xe2, 0x56, 0x66, 0xb8, 0x69, 0x4b, 0x78, 0x9a, 0x6c, 0x36, 0x66, 0x39,
    0x59, 0x71, 0x5a, 0x17, 0x35, 0x66, 0x2b, 0x9c, 0x59, 0x33, 0x3a, 0x27,
    0xf0, 0x2b, 0x56, 0xe1, 0x2b, 0xaa, 0x36, 0x4b, 0x2d, 0xa6, 0x1b, 0x65,
    0xe4, 0x33, 0xd1, 0x69, 0xc5, 0x17, 0xe4, 0xb8, 0x2e, 0x0f, 0x95, 0x2e,
    0xa5, 0x36, 0x8b, 0x53, 0xb1, 0x65, 0x71, 0x4d, 0xaa, 0x1b, 0xa3, 0x6a,
    0x1d, 0x1d, 0x78, 0x6a, 0x4d, 0x65, 0x87, 0x27, 0x2d, 0xe1, 0x55, 0x6c,
    0x8b, 0x8e, 0x99, 0x4e, 0x3c, 0x4e, 0x78, 0xe8, 0xa3, 0x9a, 0x9c, 0x99,
    0x3c, 0x5a, 0x5a, 0x4e, 0x95, 0xa3, 0x3a, 0xa5, 0xca, 0x63, 0xca, 0x96,
    0x1e, 0x27, 0xb2, 0x2e, 0x1e, 0x9c, 0x6c, 0x56, 0xb4, 0x9c, 0xc3, 0x59,
    0x87, 0x27, 0x35, 0xaa, 0xc3, 0xe2, 0xcc, 0x35, 0x0f, 0x96, 0xc6, 0xc5,
    0x3c, 0xe4, 0x87, 0x56, 0xb1, 0x8e, 0xd1, 0xb2, 0xa9, 0x69, 0x4b, 0x3a,
    0x95, 0xc6, 0xa5, 0xc9, 0x59, 0x74, 0x1e, 0xd8, 0x74, 0x69, 0x87, 0xd2,
    0x5c, 0x39, 0xd4, 0xb1, 0xc5, 0x0f, 0xa3, 0xac, 0x66, 0x2d, 0x1d, 0xd2,
    0xac, 0x5c, 0x63, 0x93, 0xc9, 0x93, 0xd8, 0xac, 0x1b, 0x47, 0xc9, 0xe1,
    0x55, 0x33, 0xb4, 0x55, 0x39, 0x65, 0x47, 0x39, 0x99, 0xe2, 0x2b, 0x2d,
    0x72, 0x6c, 0xd4, 0x8e, 0x53, 0x8d, 0x8d, 0x8b, 0x2e, 0x5c, 0x99, 0x33,
    0xf0, 0x0f, 0x74, 0xd8, 0x6a, 0x47, 0x63, 0x72, 0xa6, 0x36, 0x8b, 0x17,
    0xca, 0x4d, 0xd2, 0x6a, 0xb2, 0xf0, 0x9a, 0xcc, 0x78, 0x47, 0xe8, 0x3c,
    0xc6, 0x63, 0x53, 0xa6, 0x9a, 0x17, 0x8e, 0x4d, 0xd1, 0xe8, 0x71, 0xd4,
    0x96, 0x74, 0x72, 0xc3, 0x1d, 0x8d, 0x93, 0x95, 0x35, 0x4b, 0x1e, 0x3a,
    0x1b, 0x96, 0x71, 0x72, 0x5c, 0xa9, 0x93, 0x5a, 0xcc, 0x55, 0xb8, 0x8d,
    0xb4, 0xa9, 0x4e, 0x53,
};

// 各ビットの動きを散らすためのテーブル(互いに素)
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
// 固定版
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

class shuffleBase {
public:
    shuffleBase() = default;
    virtual ~shuffleBase() = default;

    virtual int update(int idx, int mix) = 0;
};

} // namespace ShuffleLinearCongruentInner

namespace {

// システム全体でシャッフルテーブルを共有
class shuffleSystem : public ShuffleLinearCongruentInner::shuffleBase {
public:
    shuffleSystem();
    virtual ~shuffleSystem() = default;
    virtual int update(int idx, int mix);

private:
    struct innerTbl {
        innerTbl();
        ~innerTbl();
        operator uint8_t *() { return tbl; }
        uint8_t *tbl = nullptr;
    };
    innerTbl *tbl;
};

shuffleSystem::innerTbl::innerTbl() {
    bool created = false;

    int fd = shm_open(SHARE_MEMORY_NAME, O_RDWR, 0666);
    if(fd < 0) {
        fd = shm_open(SHARE_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
        if(fd < 0) {
            fd = shm_open(SHARE_MEMORY_NAME, O_RDWR, 0666);
        } else {
            created = true;
        }
    }

    int stat = ftruncate(fd, 256);
    if(stat < 0) {
        // エラー処理は現在暫定
        throw errno;
    }

    tbl = reinterpret_cast<uint8_t *>(
        mmap(nullptr, 256, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    if(created) {
        while(getrandom(tbl, 256, GRND_NONBLOCK) > 0)
            ;
    }
}

shuffleSystem::innerTbl::~innerTbl() {
    if(tbl) munmap(tbl, 256);
}

shuffleSystem::shuffleSystem() {
    static innerTbl systemTbl;
    tbl = &systemTbl;
}

int shuffleSystem::update(int idx, int mix) {
    return (*tbl)[idx] += mix;
}

// プロセスでシャッフルテーブルを共有
class shuffleProcess : public ShuffleLinearCongruentInner::shuffleBase {
public:
    shuffleProcess();
    virtual ~shuffleProcess() = default;
    virtual int update(int idx, int mix);

private:
    struct innerTbl {
        innerTbl();
        operator uint8_t *() { return tbl; }
        uint8_t tbl[256];
    };
    innerTbl *tbl;
};

shuffleProcess::innerTbl::innerTbl() {}

shuffleProcess::shuffleProcess() {
    static innerTbl inner;
    tbl = &inner;
}

int shuffleProcess::update(int idx, int mix) {
    return (*tbl)[idx] += mix;
}

// インタンス毎にシャッフルテーブルは独立
class shufflePrivate : public ShuffleLinearCongruentInner::shuffleBase {
public:
    shufflePrivate() = default;
    virtual ~shufflePrivate() = default;
    virtual int update(int idx, int mix);

private:
    uint8_t tbl[256];
};

int shufflePrivate::update(int idx, int mix) {
    return tbl[idx] += mix;
}

std::unique_ptr<ShuffleLinearCongruentInner::shuffleBase>
getShuffle(ShuffleLinearCongruent::SHUFFLE type,
           const std::initializer_list<uint8_t> &shuffle) {
    switch(type) {
    case ShuffleLinearCongruent::SHUFFLE::SYSTEM:
        return std::make_unique<shuffleSystem>();
        break;
    case ShuffleLinearCongruent::SHUFFLE::PROCESS:
        return std::make_unique<shuffleProcess>();
        break;
    case ShuffleLinearCongruent::SHUFFLE::PRIVATE:
    default:
        return std::make_unique<shufflePrivate>();
        break;
    }
}

} // namespace

ShuffleLinearCongruent::ShuffleLinearCongruent(uint32_t seed, SHUFFLE type)
    : shuffle(getShuffle(SHUFFLE::PROCESS, {})) {
    setState({seed});
}
ShuffleLinearCongruent::ShuffleLinearCongruent(
    const std::initializer_list<uint32_t> &seed, SHUFFLE type)
    : shuffle(getShuffle(type, {})) {
    setState(seed);
}
ShuffleLinearCongruent::ShuffleLinearCongruent(
    const std::initializer_list<uint32_t> &seed,
    const std::initializer_list<uint8_t> &shuffle, SHUFFLE type)
    : shuffle(getShuffle(type, shuffle)) {
    setState(seed);
}

void ShuffleLinearCongruent::setState(
    const std::initializer_list<uint32_t> &seed) {
    if(seed.size() == 0) {
        // 初期化リストが空っぽ
        uint64_t d = baseCoef + baseCoef * 12345;
        for(auto &bs : bitState) {
            bs.setData(d);
            d += baseCoef;
        }
    } else {
        // 初期化リストから状態テーブルを設定する
        auto it = seed.begin();
        for(auto &bs : bitState) {
            bs.setData(*it++);
            if(it == seed.end())
                it = seed.begin(); // 足りなくなったら最初から再使用
        }
    }
}

// デストラクタは空でも実体が必要
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

        if(BitRndTbl[shuffle->update(idx, mix)] & (1 << (carry & 7))) {
            data |= bit;
        }
        bit <<= 1;
    }

    lastCarry = carry;

    return data;
}
