#include "../crypto.hh"
#include "../uuid.hh"

namespace ssc::runtime::uuid {
  void v7 (char* buffer) {
    static Atomic<bool> seeded = 0;
    if (!seeded) {
      srand(static_cast<unsigned int>(time(nullptr)));
      seeded = true;
    }

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t delta = ((uint64_t) tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    uint64_t timestamp = delta & ((1ull << 60) - 1);
    uint64_t value = 0;

    for (int i = 0; i < 8; ++i) {
      value = (value << 8) | (crypto::rand64() % 256);
    }
    value &= ((1ull << 62) - 1);

    uint64_t part1 = (timestamp << 4) | 0x7;
    uint64_t part2 = value & ~0xc000000000000000ull;
    part2 |= 0x8000000000000000ull;


    sprintf(
      buffer,
      "%08x-%04x-%04x-%02x%02x-%012llx",
      (uint32_t) (part1 >> 32),
      (uint16_t) (part1 >> 16),
      (uint16_t) (part1),
      (uint8_t) (part2 >> 56),
      (uint8_t) (part2 >> 48),
      static_cast<unsigned long long >(part2 & 0x0000ffffffffffffull)
    );
  }

  String v7 () {
    String output;
    output.resize(36);
    v7(output.data());
    return output;
  }
}
