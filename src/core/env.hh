#ifndef SSC_CORE_ENV_H
#define SSC_CORE_ENV_H

#include "types.hh"

namespace SSC {
  class Env {
    public:
      static bool has (const char* name);
      static bool has (const String& name);

      static String get (const char* name);
      static String get (const String& name);
      static String get (const String& name, const String& fallback);

      static void set (const String& name, const String& value);
      static void set (const char* name);
  };
}

#endif
