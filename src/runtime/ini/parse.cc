#include "../ini.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::INI {
  INI::Map parse (const String& source) {
    return parse(source, "_");
  }

  INI::Map parse (
    const String& source,
    const String& keyPathSeparator
  ) {
    Vector<String> entries = split(source, '\n');
    String prefix = "";
    INI::Map settings = {};

    for (auto entry : entries) {
      entry = trim(entry);

      // handle a variety of comment styles
      if (entry[0] == ';' || entry[0] == '#') {
        continue;
      }

      if (entry.starts_with("[") && entry.ends_with("]")) {
        if (entry.starts_with("[.") && entry.ends_with("]")) {
          prefix += entry.substr(2, entry.length() - 3);
        } else {
          prefix = entry.substr(1, entry.length() - 2);
        }

        prefix = replace(prefix, "\\.", keyPathSeparator);
        if (prefix.size() > 0) {
          prefix += keyPathSeparator;
        }

        continue;
      }

      auto index = entry.find_first_of('=');

      if (index >= 0 && index <= entry.size()) {
        auto key = trim(prefix + entry.substr(0, index));
        auto value = trim(entry.substr(index + 1));

        // trim quotes from quoted strings
        size_t closing_quote_index = -1;
        bool quoted_value = false;
        if (value[0] == '"') {
          closing_quote_index = value.find_first_of('"', 1);
          if (closing_quote_index != std::string::npos) {
            quoted_value = true;
            value = trim(value.substr(1, closing_quote_index - 1));
          }
        } else if (value[0] == '\'') {
          closing_quote_index = value.find_first_of('\'', 1);
          if (closing_quote_index != std::string::npos) {
            quoted_value = true;
            value = trim(value.substr(1, closing_quote_index - 1));
          }
        }

        if (!quoted_value) {
          // ignore comments within quoted part of value
          auto i = value.find_first_of(';');
          auto j = value.find_first_of('#');

          if (i > 0) {
            value = trim(value.substr(0, i));
          }
          else if (j > 0) {
            value = trim(value.substr(0, j));
          }
        }

        if (key.ends_with("[]")) {
          key = trim(key.substr(0, key.size() - 2));

          // handle special headers configurations
          if (key.ends_with("_headers")) {
            // inject '\n' as headers should be stored with
            // new lines for each entry in the configuration
            value += "\n";
          }

          if (settings[key].size() > 0) {
            settings[key] += " " + value;
          } else {
            settings[key] = value;
          }
        } else {
          settings[key] = value;
        }
      }
    }

    return settings;
  }
}
