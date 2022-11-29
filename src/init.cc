#include "common.hh"

//
// SSC_SETTINGS and DEBUG are compile time variables provided by the compiler.
//
namespace SSC {
  const char* getSettingsSource () {
    static const char* source = STR_VALUE(SSC_SETTINGS);
    return source;
  }

  bool isDebugEnabled () {
  #if defined(DEBUG) && DEBUG
    return true;
  #endif
    return false;
  }
}
