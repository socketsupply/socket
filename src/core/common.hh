#ifndef SSC_CORE_COMMON_H
#define SSC_CORE_COMMON_H

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#if !defined(_WIN32)
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// macOS/iOS
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#include <_types/_uint64_t.h>
#include <netinet/in.h>
#include <sys/un.h>
#else
#include <objc/objc-runtime.h>
#endif

#ifndef debug
#define debug(format, ...) NSLog(@format, ##__VA_ARGS__)
#endif
#endif

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#ifndef debug
#define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif
#endif

// Android (Linux)
#if defined(__linux__) && defined(__ANDROID__)
// Java Native Interface
// @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#ifndef debug
#define debug(format, ...) \
  __android_log_print(     \
      ANDROID_LOG_DEBUG,   \
      __FUNCTION__,        \
      format,              \
      ##__VA_ARGS__        \
    );
#endif
#endif

// Windows
#if defined(_WIN32)
#include <Windows.h>

#include <arpa/inet.h>
#include <dwmapi.h>
#include <io.h>
#include <tchar.h>
#include <wingdi.h>
#include <wrl.h>

#include <future>
#include <signal.h>
#include <shlobj_core.h>
#include <shobjidl.h>

#define ISATTY _isatty
#define FILENO _fileno

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"uv_a.lib")
#pragma comment(lib,"Gdi32.lib")
#else // !_WIN32
#include <unistd.h>
#include <sys/wait.h>
#define ISATTY isatty
#define FILENO fileno
#endif

#include <any>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <regex>
#include <semaphore>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SETTINGS
#  define SETTINGS ""
#endif

#ifndef VERSION
#  define VERSION ""
#endif

#ifndef VERSION_HASH
#  define VERSION_HASH ""
#endif

#ifndef SOCKET_MAX_WINDOWS
#define SOCKET_MAX_WINDOWS 32
#endif

#if !DEBUG
#ifdef debug
#undef debug
#endif
#define debug(format, ...)
#endif

#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

#define IN_URANGE(c, a, b) (                  \
    (unsigned char) c >= (unsigned char) a && \
    (unsigned char) c <= (unsigned char) b    \
)

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

namespace SSC {
  namespace fs = std::filesystem;
  using Map = std::map<std::string, std::string>;
  using MessageCallback = std::function<void(const std::string)>;
  using ExitCallback = std::function<void(int code)>;

  constexpr auto full_version = STR_VALUE(VERSION) " (" STR_VALUE(VERSION_HASH) ")";
  constexpr auto version_hash = STR_VALUE(VERSION_HASH);
  constexpr auto version = STR_VALUE(VERSION);

  inline std::string encodeURIComponent (const std::string& sSrc);
  inline std::string decodeURIComponent (const std::string& sSrc);
  inline std::string trim (std::string str);

  //
  // Cross platform support for strings
  //
  #if defined(_WIN32)
    using String = std::wstring;
    using Stringstream = std::wstringstream;
    using namespace Microsoft::WRL;
    #define Str(s) L##s
    #define RegExp std::wregex

    inline std::wstring StringToWString (const std::string& s) {
      std::wstring temp(s.length(), L' ');
      std::copy(s.begin(), s.end(), temp.begin());
      return temp;
    }

    inline std::string WStringToString (const std::wstring& s) {
      std::string temp(s.length(), ' ');
      std::copy(s.begin(), s.end(), temp.begin());
      return temp;
    }

  #else
    using String = std::string;
    using Stringstream = std::stringstream;
    #define RegExp std::regex
    #define Str(s) s
    #define StringToWString(s) s
    #define WStringToString(s) s
  #endif

  //
  // Reporting on the platform.
  //
  struct {
    #if defined(__x86_64__) || defined(_M_X64)
      const std::string arch = "x86_64";
    #elif defined(__aarch64__) || defined(_M_ARM64)
      const std::string arch = "arm64";
    #else
      const std::string arch = "unknown";
    #endif

    #if defined(_WIN32)
      const std::string os = "win32";
      bool mac = false;
      bool ios = false;
      bool win = true;
      bool linux = false;
      bool unix = false;

    #elif defined(__APPLE__)
      bool win = false;
      bool linux = false;

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        std::string os = "ios";
        bool ios = true;
        bool mac = false;
      #else
        std::string os = "mac";
        bool ios = false;
        bool mac = true;
      #endif

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #elif defined(__linux__)
      #undef linux
      #ifdef __ANDROID__
        const std::string os = "android";
      #else
        const std::string os = "linux";
      #endif

      bool mac = false;
      bool ios = false;
      bool win = false;
      bool linux = true;

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #elif defined(__FreeBSD__)
      const std::string os = "freebsd";
      bool mac = false;
      bool ios = false;
      bool win = false;
      bool linux = false;

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #elif defined(BSD)
      const std::string os = "openbsd";
      bool ios = false;
      bool mac = false;
      bool win = false;
      bool linux = false;

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #endif
  } platform;

  inline const std::vector<std::string>
  splitc(const std::string& s, const char& c) {
    std::string buff;
    std::vector<std::string> vec;

    for (auto n : s) {
      if (n != c) {
        buff += n;
      } else if (n == c) {
        vec.push_back(buff);
        buff = "";
      }
    }

    vec.push_back(buff);

    return vec;
  }

  inline size_t decodeUTF8 (char *output, const char *input, size_t length) {
    unsigned char cp = 0; // code point
    unsigned char lower = 0x80;
    unsigned char upper = 0xBF;

    int x = 0; // cp needed
    int y = 0; // cp  seen
    int size = 0; // output size

    for (int i = 0; i < length; ++i) {
      auto b = (unsigned char) input[i];

      if (b == 0) {
        output[size++] = 0;
        continue;
      }

      if (x == 0) {
        // 1 byte
        if (IN_URANGE(b, 0x00, 0x7F)) {
          output[size++] = b;
          continue;
        }

        if (!IN_URANGE(b, 0xC2, 0xF4)) {
          break;
        }

        // 2 byte
        if (IN_URANGE(b, 0xC2, 0xDF)) {
          x = 1;
          cp = b - 0xC0;
        }

        // 3 byte
        if (IN_URANGE(b, 0xE0, 0xEF)) {
          if (b == 0xE0) {
            lower = 0xA0;
          } else if (b == 0xED) {
            upper = 0x9F;
          }

          x = 2;
          cp = b - 0xE0;
        }

        // 4 byte
        if (IN_URANGE(b, 0xF0, 0xF4)) {
          if (b == 0xF0) {
            lower = 0x90;
          } else if (b == 0xF4) {
            upper = 0x8F;
          }

          x = 3;
          cp = b - 0xF0;
        }

        cp = cp * pow(64, x);
        continue;
      }

      if (!IN_URANGE(b, lower, upper)) {
        lower = 0x80;
        upper = 0xBF;

        // revert
        cp = 0;
        x = 0;
        y = 0;
        i--;
        continue;
      }

      lower = 0x80;
      upper = 0xBF;
      y++;
      cp += (b - 0x80) * pow(64, x - y);

      if (y != x) {
        continue;
      }

      output[size++] = cp;
      // continue to next
      cp = 0;
      x = 0;
      y = 0;
    }

    return size;
  }

  template <typename ...Args> std::string format (const std::string& s, Args ...args) {
    auto copy = s;
    std::stringstream res;
    std::vector<std::any> vec;
    using unpack = int[];

    (void) unpack { 0, (vec.push_back(args), 0)... };

    std::regex re("\\$[^$\\s]");
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
      } else if (match.str() == "$C") {
        auto value = std::any_cast<char*>(vec[index++]);
        copy = std::regex_replace(copy, re, std::string(value), first);
      } else if (match.str() == "$c") {
        auto value = std::any_cast<char>(vec[index++]);
        copy = std::regex_replace(copy, re, std::string(1, value), first);
      } else {
        copy = std::regex_replace(copy, re, match.str(), first);
      }
    }

    return copy;
  }

  inline std::string replace (const std::string& src, const std::string& re, const std::string& val) {
    return std::regex_replace(src, std::regex(re), val);
  }

  inline std::string& replaceAll (std::string& src, std::string const& from, std::string const& to) {
    size_t start = 0;
    size_t index;

    while ((index = src.find(from, start)) != std::string::npos) {
      src.replace(index, from.size(), to);
      start = index + to.size();
    }
    return src;
  }

  inline std::string emitToRenderProcess (const std::string& event, const std::string& value) {
    return std::string(
      ";(() => {\n"
      "  const name = decodeURIComponent(`" + event + "`);\n"
      "  const value = `" + value + "`;\n"
      "  window._ipc.emit(name, value);\n"
      "})();\n"
      "//# sourceURL=emit-to-render-process.js\n"
    );
  }

  inline std::string resolveMenuSelection (const std::string& seq, const std::string& title, const std::string& parent) {
    return std::string(
      ";(() => {\n"
      "  const detail = {\n"
      "    title: decodeURIComponent(`" + title + "`),\n"
      "    parent: decodeURIComponent(`" + parent + "`),\n"
      "    state: '0'\n"
      "  };\n"

      "  if (" + seq + " > 0 && window._ipc['R" + seq + "']) {\n"
      "    window._ipc['R" + seq + "'].resolve(detail);\n"
      "    delete window._ipc['R" + seq + "'];\n"
      "    return;\n"
      "  }\n"

      "  const event = new window.CustomEvent('menuItemSelected', { detail });\n"
      "  window.dispatchEvent(event);\n"
      "})();\n"
      "//# sourceURL=resolve-menu-selection.js\n"
    );
  }

  //
  // Helper functions...
  //
  inline const std::vector<std::string> split (const std::string& s, const char& c) {
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

  inline std::string trim (std::string str) {
    str.erase(0, str.find_first_not_of(" \r\n\t"));
    str.erase(str.find_last_not_of(" \r\n\t") + 1);
    return str;
  }

  inline std::string tmpl (const std::string s, Map pairs) {
    std::string output = s;

    for (auto item : pairs) {
      auto key = std::string("[{]+(" + item.first + ")[}]+");
      auto value = item.second;
      output = std::regex_replace(output, std::regex(key), value);
    }

    return output;
  }

  inline uint64_t rand64(void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
      r <<= RAND_MAX_WIDTH;
      r ^= (unsigned) rand();
    }
    return r;
  }

  inline std::string getEnv(const char* variableName) {
    #if _WIN32
      char* variableValue = nullptr;
      std::size_t valueSize = 0;
      auto query = _dupenv_s(&variableValue, &valueSize, variableName);

      std::string result;
      if(query == 0 && variableValue != nullptr && valueSize > 0) {
        result.assign(variableValue, valueSize - 1);
        free(variableValue);
      }

      return result;
    #else
      auto v = getenv(variableName);

      if (v != nullptr) {
        return std::string(v);
      }

      return std::string("");
    #endif
  }

  inline auto setEnv(const char* s) {
    #if _WIN32
      return _putenv(s);
    #else

      return putenv((char*) &s[0]);
    #endif
  }

  inline void stdWrite(const std::string &str, bool isError) {
    #ifdef _WIN32
      std::stringstream ss;
      ss << str << std::endl;
      auto lineStr = ss.str();

      auto handle = isError ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
      WriteConsoleA(GetStdHandle(handle), lineStr.c_str(), lineStr.size(), NULL, NULL);
    #else
      (isError ? std::cerr : std::cout) << str << std::endl;
    #endif
  }

  #if !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    inline String readFile (fs::path path) {
      std::ifstream stream(path.c_str());
      String content;
      auto buffer = std::istreambuf_iterator<char>(stream);
      auto end = std::istreambuf_iterator<char>();
      content.assign(buffer, end);
      stream.close();
      return content;
    }

    inline void writeFile (fs::path path, std::string s) {
      std::ofstream stream(path.string());
      stream << s;
      stream.close();
    }

    inline void appendFile (fs::path path, std::string s) {
      std::ofstream stream;
      stream.open(path.string(), std::ios_base::app);
      stream << s;
      stream.close();
    }
  #endif

  inline std::string prefixFile (std::string s) {
    if (platform.mac || platform.linux) {
      std::string local = getEnv("HOME");
      return std::string(local + "/.config/socket-sdk/" + s + " ");
    }

    std::string local = getEnv ("LOCALAPPDATA");
    return std::string(local + "\\Programs\\socketsupply\\" + s + " ");
  }

  inline std::string prefixFile () {
    if (platform.mac || platform.linux) {
      std::string local = getEnv("HOME");
      return std::string(local + "/.config/socket-sdk/");
    }

    std::string local = getEnv ("LOCALAPPDATA");
    return std::string(local + "\\Programs\\socketsupply");
  }

  inline Map parseConfig (std::string source) {
    auto entries = split(source, '\n');
    Map settings;

    for (auto entry : entries) {
      auto index = entry.find_first_of(':');

      if (index >= 0 && index <= entry.size()) {
        auto key = entry.substr(0, index);
        auto value = entry.substr(index + 1);

        settings[trim(key)] = trim(value);
      }
    }

    return settings;
  }

  //
  // IPC Message parser for the middle end
  // TODO possibly harden data validation.
  //
  class Parse {
    Map args;
    public:
      Parse(const std::string&);
      int index = -1;
      std::string value = "";
      std::string name = "";
      std::string uri = "";
      std::string get(const std::string&) const;
      std::string get(const std::string&, const std::string) const;
      const char * c_str () const {
        return this->uri.c_str();
      }
  };

  struct ScreenSize {
    int height = 0;
    int width = 0;
  };

  //
  // cmd: `ipc://id?p1=v1&p2=v2&...\0`
  //
  inline Parse::Parse (const std::string& s) {
    std::string str = s;
    uri = str;

    // bail if missing protocol prefix
    if (str.find("ipc://") == -1) return;

    // bail if malformed
    if (str.compare("ipc://") == 0) return;
    if (str.compare("ipc://?") == 0) return;

    std::string query;
    std::string path;

    auto raw = split(str, '?');
    path = raw[0];
    if (raw.size() > 1) query = raw[1];

    auto parts = split(path, '/');
    if (parts.size() >= 1) name = parts[1];

    if (raw.size() != 2) return;
    auto pairs = split(raw[1], '&');

    for (auto& rawPair : pairs) {
      auto pair = split(rawPair, '=');
      if (pair.size() <= 1) continue;

      if (pair[0].compare("index") == 0) {
        try {
          index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
        } catch (...) {
          std::cout << "Warning: received non-integer index" << std::endl;
        }
      }

      args[pair[0]] = pair[1];
    }
  }

  inline std::string Parse::get(const std::string& s) const {
    return args.count(s) ? args.at(s): "";
  }

  inline std::string Parse::get(const std::string& s, const std::string fallback) const {
    return args.count(s) ? args.at(s) : fallback;
  }

  //
  // All ipc uses a URI schema, so all ipc data needs to be
  // encoded as a URI component. This prevents escaping the
  // protocol.
  //
  static const signed char HEX2DEC[256] = {
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
  };

  inline std::string decodeURIComponent(const std::string& sSrc) {

    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    auto s = replace(sSrc, "\\+", " ");
    const unsigned char* pSrc = (const unsigned char *) s.c_str();
    const int SRC_LEN = (int) sSrc.length();
    const unsigned char* const SRC_END = pSrc + SRC_LEN;
    const unsigned char* const SRC_LAST_DEC = SRC_END - 2;

    char* const pStart = new char[SRC_LEN];
    char* pEnd = pStart;

    while (pSrc < SRC_LAST_DEC) {
      if (*pSrc == '%') {
        char dec1, dec2;
        if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
            && -1 != (dec2 = HEX2DEC[*(pSrc + 2)])) {

            *pEnd++ = (dec1 << 4) + dec2;
            pSrc += 3;
            continue;
        }
      }
      *pEnd++ = *pSrc++;
    }

    // the last 2- chars
    while (pSrc < SRC_END) {
      *pEnd++ = *pSrc++;
    }

    std::string sResult(pStart, pEnd);
    delete [] pStart;
    return sResult;
  }

  static const char SAFE[256] = {
      /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
      /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

      /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
      /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
      /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
      /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

      /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

      /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  };

  inline std::string encodeURIComponent (const std::string& sSrc) {
    const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
    const unsigned char* pSrc = (const unsigned char*) sSrc.c_str();
    const int SRC_LEN = (int) sSrc.length();
    unsigned char* const pStart = new unsigned char[SRC_LEN* 3];
    unsigned char* pEnd = pStart;
    const unsigned char* const SRC_END = pSrc + SRC_LEN;

    for (; pSrc < SRC_END; ++pSrc) {
      if (SAFE[*pSrc]) {
        *pEnd++ = *pSrc;
      } else {
        // escape this char
        *pEnd++ = '%';
        *pEnd++ = DEC2HEX[*pSrc >> 4];
        *pEnd++ = DEC2HEX[*pSrc & 0x0F];
      }
    }

    std::string sResult((char*) pStart, (char*) pEnd);
    delete [] pStart;
    return sResult;
  }

  inline std::string resolveToRenderProcess (const std::string& seq, const std::string& state, const std::string& value) {
    return std::string(
      "(() => {"
      "  const seq = String('" + seq + "');"
      "  const state = Number('" + state + "');"
      "  const value = '" + value + "';"
      "  window._ipc.resolve(seq, state, value);"
      "})()"
    );
  }

  inline std::string resolveToMainProcess (const std::string& seq, const std::string& state, const std::string& value) {
    return std::string("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }
}

#endif // SSC_H
