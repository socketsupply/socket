#ifndef SOCKET_RUNTIME_STRING_H
#define SOCKET_RUNTIME_STRING_H

#include <regex>
#include "platform.hh"

/**
 * Converts a literal expression to an inline string:
 * `CONVERT_TO_STRING(this_value)` becomes "this_value"
 */
#define _CONVERT_TO_STRING(value) #value
#define CONVERT_TO_STRING(value) _CONVERT_TO_STRING(value)

namespace ssc::runtime::string {
  // transform
  String replace (const String& source, const String& regex, const String& value);
  String replace (const String& source, const std::regex& regex, const String& value);
  String tmpl (const String& source, const Map<String, String>& variables);
  String trim (String source);
  String toLowerCase (const String& source);
  String toUpperCase (const String& source);
  String toProperCase (const String& source);

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
  const String join (const Set<String>& set, const String& separator);
  const String join (const Set<String>& set, const char separator);
  Vector<String> parseStringList (const String& string, const Vector<char>& separators);
  Vector<String> parseStringList (const String& string, const char separator);
  Vector<String> parseStringList (const String& string);

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  String formatWindowsError (DWORD error, const String& source);
#endif
}
#endif
