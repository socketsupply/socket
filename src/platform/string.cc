#include "platform.hh"
#include "string.hh"

#if defined(min)
#undef min
#endif

namespace SSC {
  String replace (const String& source, const std::regex& regex, const String& value) {
    return std::regex_replace(source, regex, value);
  }

  String replace (const String& source, const String& regex, const String& value) {
    return replace(source, std::regex(regex), value);
  }

  String tmpl (const String& source, const Map& variables) {
    auto output = source;

    for (const auto& tuple : variables) {
      auto key = String("[{]+(" + tuple.first + ")[}]+");
      auto value = tuple.second;
      output = std::regex_replace(output, std::regex(key), value);
    }

    return output;
  }

  const Vector<String> split (const String& source, const String& needle) {
    Vector<String> result;
    String current = source;
    size_t position = 0;

    while (current.size() > 0 && position < source.size()) {
      position = current.find(needle);
      if (position == String::npos) {
        result.push_back(current);
        break;
      }

      const auto string = current.substr(0, position);
      current = current.substr(std::min(current.size(), position + needle.size()));
      result.push_back(string);
      position += needle.size();
    }

    return result;
  }

  const Vector<String> split (const String& source, const char character) {
    String buffer;
    Vector<String> result;

    for (const auto current : source) {
      if (current != character) {
        buffer += current;
      } else if (current == character && buffer != "") {
        result.push_back(buffer);
        buffer = "";
      }
    }

    if (!buffer.empty()) {
      result.push_back(buffer);
    }

    return result;
  }

  const Vector<String> splitc (const String& source, const char character) {
    String buffer;
    Vector<String> result;

    for (const auto current : source) {
      if (current != character) {
        buffer += current;
      } else if (current == character) {
        result.push_back(buffer);
        buffer = "";
      }
    }

    result.push_back(buffer);

    return result;
  }

  String trim (String source) {
    source.erase(0, source.find_first_not_of(" \r\n\t"));
    source.erase(source.find_last_not_of(" \r\n\t") + 1);
    return source;
  }

  String toLowerCase (const String& source) {
    String output = source;
    std::transform(
      output.begin(),
      output.end(),
      output.begin(),
      [](auto ch) { return std::tolower(ch); }
    );
    return output;
  }

  String toUpperCase (const String& source) {
    String output = source;
    std::transform(
      output.begin(),
      output.end(),
      output.begin(),
      [](auto ch) { return std::toupper(ch); }
    );
    return output;
  }

  String toProperCase (const String& source) {
    String output = "";
    if (source.size() > 0) {
      output += toupper(source[0]);
    }

    if (source.size() > 1) {
      output += source.substr(1);
    }

    return output;
  }

  WString convertStringToWString (const String& source) {
    WString result(source.length(), L' ');
    std::copy(source.begin(), source.end(), result.begin());
    return result;
  }

  WString convertStringToWString (const WString& source) {
    return source;
  }

  String convertWStringToString (const WString& source) {
    String result(source.length(), ' ');
    std::copy(source.begin(), source.end(), result.begin());
    return result;
  }

  String convertWStringToString (const String& source) {
    return source;
  }

  const String join (const Vector<String>& vector, const String& separator) {
    auto missing = vector.size();
    StringStream joined;

    for (const auto& item : vector) {
      joined << item;
      if (--missing > 0) {
        joined << separator;
      }
    }

    return trim(joined.str());
  }

  const String join (const Vector<String>& vector, const char separator) {
    return join(vector, String(1, separator));
  }

  const String join (const Set<String>& set, const String& separator) {
    return join(Vector<String>(set.begin(), set.end()), separator);
  }

  const String join (const Set<String>& set, const char separator) {
    return join(set, String(1, separator));
  }

  Vector<String> parseStringList (const String& string, const Vector<char>& separators) {
    auto list = Vector<String>();
    for (const auto& separator : separators) {
      for (const auto& part: split(string, separator)) {
        if (std::find(list.begin(), list.end(), part) == list.end()) {
          list.push_back(part);
        }
      }
    }

    return list;
  }

  Vector<String> parseStringList (const String& string, const char separator) {
    return split(string, separator);
  }

  Vector<String> parseStringList (const String& string) {
    return parseStringList(string, { ' ', ',' });
  }

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  String formatWindowsError (DWORD error, const String& source) {
    StringStream message;
    LPVOID errorMessage;

    // format
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      error,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &errorMessage,
      0,
      nullptr
    );

    // create output string
    message
      << "Error " << error
      << " in " << source
      << ": " <<  (LPTSTR) errorMessage;

    LocalFree(lpMsgBuf);

    return message.str();
  }
#endif
}
