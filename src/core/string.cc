#include "string.hh"
#include "debug.hh"

#include <regex>

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
    String output = source;

    for (const auto tuple : variables) {
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
      if (position == std::string::npos) {
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
    return join(Vector<String>(set.begin(), set.end()), separator);
  }

  const String concat (const String& left, const String& right) {
    return left + right;
  }

  const String concat (const String& left, const char right) {
    return left + String(1, right);
  }

  const Vector<String> concat (const Vector<String>& left, const Vector<String>& right) {
    auto result = Vector<String>(left.begin(), left.end());
    result.insert(
      result.end(),
      std::make_move_iterator(right.begin()),
      std::make_move_iterator(right.end())
    );
    return result;
  }

  const Set<String> concat (const Set<String>& left, const Set<String>& right) {
    Set<String> result;
    for (const auto& item : left) {
      result.insert(item);
    }

    for (const auto& item : right) {
      result.insert(item);
    }
    return result;
  }

  const Vector<String> concat (const Vector<String>& left, const Set<String>& right) {
    return concat(left, Vector<String>(right.begin(), right.end()));
  }

  const Set<String> concat (const Set<String>& left, const Vector<String>& right) {
    return concat(left, Set<String>(right.begin(), right.end()));
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

  bool contains (const String& source, const String& search) {
    return source.find(search) != String::npos;
  }

  bool contains (const Vector<String>& source, const String& search) {
    const auto cursor = std::find(source.begin(), source.end(), search);
    return cursor != source.end();
  }

}
