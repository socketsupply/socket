#include "../crypto.hh"

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

namespace ssc::runtime::crypto {
  uint64_t rand64 () {
    static const auto width = RAND_MAX_WIDTH;
    static bool init = false;

    if (!init) {
      init = true;
      srand(time(0));
    }

    uint64_t r = 0;
    for (int i = 0; i < 64; i += width) {
      r <<= width;
      r ^= (unsigned) rand();
    }
    return r;
  }
}
