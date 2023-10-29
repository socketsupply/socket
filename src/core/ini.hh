#ifndef SSC_CORE_INI_H
#define SSC_CORE_INI_H

#include "string.hh"
#include "types.hh"

namespace SSC::INI {
  Map parse (const String& source);
  Map parse (const String& source, const String& keyPathSeparator);
}
#endif
