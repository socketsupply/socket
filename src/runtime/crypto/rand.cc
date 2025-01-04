#include <random>

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

	int randint (int a, int b) {
    if (a == 0 && b == 0) {
      return 0;
    }

    static std::random_device rd;  // non-deterministic random seed
    static std::mt19937 gen(rd()); // mersenne twister rng

    // Create a uniform distribution in the range of valid indices
    std::uniform_int_distribution<size_t> dist(a, b);

    // Generate and return a random index
    return dist(gen);
  }

	int randint (int a) {
    return randint(a, INT_MAX);
  }

	int randint () {
    return randint(0, INT_MAX);
  }
}
