#include "../ini.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::INI {
  String serialize (const Map& map) {
    StringStream stream;
    for (const auto& entry : map) {
      stream << entry.first << " = " << entry.second << "\n";
    }
    return trim(stream.str());
  }
}
