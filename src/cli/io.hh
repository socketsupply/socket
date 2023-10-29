#ifndef SSC_CLI_IO_H
#define SSC_CLI_IO_H

#include "../core/core.hh"

namespace SSC::CLI {
  String readFile (const Path& path);
  void writeFile (const Path& path, const String& string);
  void appendFile (const Path& path, const String& string);
}
#endif
