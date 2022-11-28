#include "../common.hh"

namespace SSC {
  const char* getSettingsSource () {
    static const char* source = STR_VALUE(SSC_SETTINGS);
    return source;
  }
}
