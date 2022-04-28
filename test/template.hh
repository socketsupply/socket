#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <any>

template <typename ...Args> std::string format (const std::string& s, Args ...args) {
  auto copy = s;
  std::stringstream res;
  std::vector<std::any> vec;
  using unpack = int[];

  (void) unpack { 0, (vec.push_back(args), 0)... };

  std::regex re("\\$[^$]");
  std::smatch match;
  auto first = std::regex_constants::format_first_only;
  int index = 0;

  while (std::regex_search(copy, match, re) != 0) {
    if (match.str() == "$S") {
      auto value = std::any_cast<std::string>(vec[index++]);
      copy = std::regex_replace(copy, re, value, first);
    } else if (match.str() == "$i") {
      auto value = std::any_cast<int>(vec[index++]);
      copy = std::regex_replace(copy, re, std::to_string(value), first);
    } else if (match.str() == "$c") {
      auto value = std::any_cast<char>(vec[index++]);
      copy = std::regex_replace(copy, re, std::string(1, value), first);
    } else {
      copy = std::regex_replace(copy, re, match.str(), first);
    }
  }

  return copy;
}
