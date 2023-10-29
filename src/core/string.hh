#ifndef SSC_CORE_STRING_HH
#define SSC_CORE_STRING_HH

#include "types.hh"
#include <regex>

/**
 * Converts a literal expression to an inline string:
 * `CONVERT_TO_STRING(this_value)` becomes "this_value"
 */
#define _CONVERT_TO_STRING(value) #value
#define CONVERT_TO_STRING(value) _CONVERT_TO_STRING(value)

namespace SSC {
  // transform
  String replace (const String& source, const String& regex, const String& value);
  String replace (const String& source, const std::regex& regex, const String& value);
  String tmpl (const String& source, const Map& variables);
  String trim (String source);

  // conversion
  WString convertStringToWString (const String& source);
  WString convertStringToWString (const WString& source);
  String convertWStringToString (const WString& source);
  String convertWStringToString (const String& source);

  // vector parsers
  const Vector<String> splitc (const String& source, const char character);
  const Vector<String> split (const String& source, const char character);
  const Vector<String> split (const String& source, const String& needle);
  const String join (const Vector<String>& vector, const String& separator);
  const String join (const Vector<String>& vector, const char separator);
  Vector<String> parseStringList (const String& string, const Vector<char>& separators);
  Vector<String> parseStringList (const String& string, const char separator);
  Vector<String> parseStringList (const String& string);
}

#endif
