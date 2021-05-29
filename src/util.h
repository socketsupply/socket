#ifndef OPKIT_UTIL_HPP_
#define OPKIT_UTIL_HPP_

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <chrono>
#include <thread>
#include <span>
#include <filesystem>

#include <memory>
#include <mutex>

#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define OS_WIN32
#endif

#ifdef __APPLE__
#define OS_DARWIN
#endif

#ifdef __linux__
#define OS_LINUX
#endif

struct {
#ifdef OS_LINUX
  bool darwin = false;
  bool win32 = false;
  bool linux = true;
  const std::string os = "linux";
#endif

#ifdef OS_DARWIN
  bool darwin = true;
  bool win32 = false;
  bool linux = false;
  const std::string os = "darwin";
#endif

#ifdef OS_WIN32
  bool darwin = false;
  bool win32 = true;
  bool linux = false;
  const std::string os = "win32";
#endif
} platform;

namespace fs = std::filesystem;

enum {
  NOC_FILE_DIALOG_OPEN    = 1 << 0,   // Create an open file dialog.
  NOC_FILE_DIALOG_SAVE    = 1 << 1,   // Create a save file dialog.
  NOC_FILE_DIALOG_DIR     = 1 << 2,   // Open a directory.
  NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION = 1 << 3,
};

std::string createNativeDialog(
  int flags,
  const char *filters,
  const char *default_path,
  const char *default_name);

inline std::string pathToString(const fs::path &path) {
  auto s = path.u8string();
  return std::string(s.begin(), s.end());
}

inline const std::vector<std::string> split(const std::string& s, const char& c) {
  std::string buff;
  std::vector<std::string> vec;
	
	for (auto n : s) {
		if(n != c) {
      buff += n;
    } else if (n == c && buff != "") {
      vec.push_back(buff);
      buff = "";
    }
	}

	if (!buff.empty()) vec.push_back(buff);

	return vec;
}

inline std::string& trim(std::string& str) {
  str.erase(0, str.find_first_not_of(" \r\n\t"));
  str.erase(str.find_last_not_of(" \r\n\t") + 1);
  return str;
}

inline std::string tmpl(const std::string s, std::map<std::string, std::string> pairs) {
  std::string output = s;

  for (auto item : pairs) {
    auto key = "[{]+(" + item.first + ")[}]+";
    auto value = item.second;
    output = std::regex_replace(output, std::regex(key), value);
  }

  return output;
}

inline std::string replace(std::string src, std::string re, std::string val) {
  return std::regex_replace(src, std::regex(re), val);
}

inline std::string exec(std::string command) {
  FILE *pipe;
  char buf[128];

#ifdef _WIN32
  //
  // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen?view=msvc-160
  // _popen works fine in a console application... ok fine that's all we need it for... thanks.
  //
  pipe = _popen((const char*) command.c_str(), "rt");
#else
  pipe = popen((const char*) command.c_str(), "r");
#endif

  if (pipe == NULL) {
    std::cout << "error: unable to opent he command" << std::endl;
    exit(1);
  }

  std::stringstream ss;

  while (fgets(buf, 128, pipe)) {
    ss << buf;
  }

#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif

  return ss.str();
}

inline std::string readFile(fs::path path) {
  std::ifstream stream(path.c_str());
  std::string content;
  auto buffer = std::istreambuf_iterator<char>(stream);
  auto end = std::istreambuf_iterator<char>();
  content.assign(buffer, end);
  stream.close();
  return content;
}

inline void writeFile (fs::path path, std::string s) {
  std::ofstream stream(pathToString(path));
  stream << s;
  stream.close();
}

inline std::map<std::string, std::string> parseConfig(std::string source) {
  auto entries = split(source, '\n');
  std::map<std::string, std::string> settings;

  for (auto entry : entries) {
    auto pair = split(entry, ':');

    if (pair.size() == 2) {
      settings[trim(pair[0])] = trim(pair[1]);
    }
  }

  return settings;
}

//
// Help decide where build source files will be placed
//
inline std::string prefixFile(std::string s) {
  if (platform.darwin || platform.linux) {
    return std::string("/usr/local/lib/opkit/" + s + " ");
  }

  return std::string("C:\\Program Files\\operator\\build\\" + s + " ");
}

#endif // OPKIT_UTIL_HPP_
