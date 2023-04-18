#ifndef SSC_CORE_COMMON_H
#define SSC_CORE_COMMON_H

// macOS/iOS
#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <OSLog/OSLog.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#include <_types/_uint64_t.h>
#include <netinet/in.h>
#include <sys/un.h>
#else
#include <objc/objc-runtime.h>
#endif

#ifndef debug
#if !defined(SSC_CLI)
static os_log_t SSC_OS_LOG_DEBUG_BUNDLE = nullptr;
// wrap `os_log*` functions for global debugger
#define osdebug(format, fmt, ...) ({                                           \
  if (!SSC_OS_LOG_DEBUG_BUNDLE) {                                              \
    static auto userConfig = SSC::getUserConfig();                             \
    static auto bundleIdentifier = userConfig["meta_bundle_identifier"];       \
    SSC_OS_LOG_DEBUG_BUNDLE = os_log_create(                                   \
      bundleIdentifier.c_str(),                                                \
      "socket.runtime.debug"                                                   \
    );                                                                         \
  }                                                                            \
                                                                               \
  auto string = [NSString stringWithFormat: @fmt, ##__VA_ARGS__];              \
  os_log_with_type(                                                            \
    SSC_OS_LOG_DEBUG_BUNDLE,                                                   \
    OS_LOG_TYPE_ERROR,                                                         \
    "%{public}s",                                                              \
    string.UTF8String                                                          \
  );                                                                           \
})
#else
#define osdebug(...)
#endif

#define debug(format, ...) ({                                                  \
  NSLog(@format, ##__VA_ARGS__);                                               \
  osdebug("%{public}@", format, ##__VA_ARGS__);                                \
})
#endif
#endif

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#ifndef debug
#define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif
#endif

#if defined(_WIN32) && defined(DEBUG)
#define _WIN32_DEBUG 1
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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#undef _WINSOCKAPI_
#define _WINSOCKAPI_

#include <WinSock2.h>
#include <windows.h>

#include <dwmapi.h>
#include <io.h>
#include <tchar.h>
#include <wingdi.h>

#include <future>
#include <signal.h>
#include <shlobj_core.h>
#include <shobjidl.h>

#define isatty _isatty
#define fileno _fileno

#ifndef debug
#define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif
#endif

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <any>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SSC_SETTINGS
#define SSC_SETTINGS ""
#endif

#ifndef SSC_VERSION
#define SSC_VERSION ""
#endif

#ifndef SSC_VERSION_HASH
#define SSC_VERSION_HASH ""
#endif

#ifndef HOST
#define HOST "localhost"
#endif

#ifndef PORT
#define PORT 0
#endif

      /*
#if !DEBUG
#ifdef debug
#undef debug
#endif
#define debug(format, ...)
#endif
*/
#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

#define IN_URANGE(c, a, b) (                  \
    (unsigned char) c >= (unsigned char) a && \
    (unsigned char) c <= (unsigned char) b    \
)

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

#define ToWString(string) WString(L##string)
#define ToString(string) String(string)

namespace SSC {
  namespace fs = std::filesystem;

  using String = std::string;
  using StringStream = std::stringstream;
  using WString = std::wstring;
  using WStringStream = std::wstringstream;
#if !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
  using Path = fs::path;
#endif

  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;

  using ExitCallback = std::function<void(int code)>;
  using Map = std::map<String, String>;
  using Mutex = std::recursive_mutex;
  using Lock = std::lock_guard<Mutex>;
  using MessageCallback = std::function<void(const String)>;
  using Thread = std::thread;

  inline const auto VERSION_FULL_STRING = ToString(STR_VALUE(SSC_VERSION) " (" STR_VALUE(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = ToString(STR_VALUE(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = ToString(STR_VALUE(SSC_VERSION));

  inline const auto DEFAULT_SSC_RC_FILENAME = String(".sscrc");
  inline const auto DEFAULT_SSC_ENV_FILENAME = String(".ssc.env");

  const Map getUserConfig ();

  bool isDebugEnabled ();

  const char* getDevHost ();
  int getDevPort ();

  inline String encodeURIComponent (const String& sSrc);
  inline String decodeURIComponent (const String& sSrc);
  inline String trim (String str);

  inline WString StringToWString (const String& s) {
    WString temp(s.length(), L' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
  }

  inline WString StringToWString (const WString& s) {
    return s;
  }

  inline String WStringToString (const WString& s) {
    String temp(s.length(), ' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
  }

  inline String WStringToString (const String& s) {
    return s;
  }

  //
  // Reporting on the platform.
  //
  static struct {
    #if defined(__x86_64__) || defined(_M_X64)
      const String arch = "x86_64";
    #elif defined(__aarch64__) || defined(_M_ARM64)
      const String arch = "arm64";
    #elif defined(__i386__) && !defined(__ANDROID__)
      #error Socket is not supported on i386.
    #else
      const String arch = "unknown";
    #endif

    #if defined(_WIN32)
      const String os = "win32";
      bool mac = false;
      bool ios = false;
      bool win = true;
      bool linux = false;
      bool unix = false;

    #elif defined(__APPLE__)
      bool win = false;
      bool linux = false;

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        String os = "ios";
        bool ios = true;
        bool mac = false;
      #else
        String os = "mac";
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
        const String os = "android";
      #else
        const String os = "linux";
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
      const String os = "freebsd";
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
      const String os = "openbsd";
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

  inline const Vector<String> splitc (const String& s, const char& c) {
    String buff;
    Vector<String> vec;

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

  template <typename ...Args> String format (const String& s, Args ...args) {
    auto copy = s;
    StringStream res;
    Vector<std::any> vec;
    using unpack = int[];

    (void) unpack { 0, (vec.push_back(args), 0)... };

    std::regex re("\\$[^$\\s]");
    std::smatch match;
    auto first = std::regex_constants::format_first_only;
    int index = 0;

    while (std::regex_search(copy, match, re) != 0) {
      if (match.str() == "$S") {
        auto value = std::any_cast<String>(vec[index++]);
        copy = std::regex_replace(copy, re, value, first);
      } else if (match.str() == "$i") {
        auto value = std::any_cast<int>(vec[index++]);
        copy = std::regex_replace(copy, re, std::to_string(value), first);
      } else if (match.str() == "$C") {
        auto value = std::any_cast<char*>(vec[index++]);
        copy = std::regex_replace(copy, re, String(value), first);
      } else if (match.str() == "$c") {
        auto value = std::any_cast<char>(vec[index++]);
        copy = std::regex_replace(copy, re, String(1, value), first);
      } else {
        copy = std::regex_replace(copy, re, match.str(), first);
      }
    }

    return copy;
  }

  inline String replace (const String& src, const String& re, const String& val) {
    return std::regex_replace(src, std::regex(re), val);
  }

  inline String& replaceAll (String& src, String const& from, String const& to) {
    size_t start = 0;
    size_t index;

    while ((index = src.find(from, start)) != String::npos) {
      src.replace(index, from.size(), to);
      start = index + to.size();
    }
    return src;
  }

  //
  // Helper functions...
  //
  inline const Vector<String> split (const String& s, const char& c) {
    String buff;
    Vector<String> vec;

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

  inline const Vector<int> splitToInts (const String& s, const char& c) {
    std::vector<int> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, c)) {
      result.push_back(std::stoi(token));
    }
    return result;
  }

  inline String trim (String str) {
    str.erase(0, str.find_first_not_of(" \r\n\t"));
    str.erase(str.find_last_not_of(" \r\n\t") + 1);
    return str;
  }

  inline String tmpl (const String s, Map pairs) {
    String output = s;

    for (auto item : pairs) {
      auto key = String("[{]+(" + item.first + ")[}]+");
      auto value = item.second;
      output = std::regex_replace(output, std::regex(key), value);
    }

    return output;
  }

  inline uint64_t rand64 (void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
      r <<= RAND_MAX_WIDTH;
      r ^= (unsigned) rand();
    }
    return r;
  }

  inline String getEnv (const char* variableName) {
    #if _WIN32
      char* variableValue = nullptr;
      std::size_t valueSize = 0;
      auto query = _dupenv_s(&variableValue, &valueSize, variableName);

      String result;
      if(query == 0 && variableValue != nullptr && valueSize > 0) {
        result.assign(variableValue, valueSize - 1);
        free(variableValue);
      }

      return result;
    #else
      auto v = getenv(variableName);

      if (v != nullptr) {
        return String(v);
      }

      return String("");
    #endif
  }

  inline String getEnv (const String& variableName) {
    return getEnv(variableName.c_str());
  }

  inline auto setEnv (const String& k, const String& v) {
  #if _WIN32
    return _putenv((k + "=" + v).c_str());
  #else
    setenv(k.c_str(), v.c_str(), 1);
  #endif
  }

  inline auto setEnv (const char* s) {
  #if _WIN32
    return _putenv(s);
  #else
    auto parts = split(String(s), '=');
    setEnv(parts[0], parts[1]);
  #endif
  }

  inline void notifyCli () {
  #if !defined(_WIN32)
    static auto ppid = getEnv("SSC_CLI_PID");
    static auto pid = ppid.size() > 0 ? std::stoi(ppid) : 0;
    if (pid > 0) {
      kill(pid, SIGUSR1);
    }
  #endif
  }

  inline void stdWrite (const String &str, bool isError) {
    (isError ? std::cerr : std::cout) << str << std::endl;
  #ifdef _WIN32
    StringStream ss;
    ss << str << std::endl;
    auto lineStr = ss.str();
    auto handle = isError ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
    WriteConsoleA(GetStdHandle(handle), lineStr.c_str(), lineStr.size(), NULL, NULL);
  #endif

    notifyCli();
  }

  #if !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    inline String readFile (fs::path path) {
      if (fs::is_directory(path)) {
        stdWrite("WARNING: trying to read a directory as a file: " + path.string(), true);
        return "";
      }

      std::ifstream stream(path.c_str());
      String content;
      auto buffer = std::istreambuf_iterator<char>(stream);
      auto end = std::istreambuf_iterator<char>();
      content.assign(buffer, end);
      stream.close();
      return content;
    }

    inline void writeFile (fs::path path, String s) {
      std::ofstream stream(path.string());
      stream << s;
      stream.close();
    }

    inline void appendFile (fs::path path, String s) {
      std::ofstream stream;
      stream.open(path.string(), std::ios_base::app);
      stream << s;
      stream.close();
    }
  #endif

  inline Map& extendMap (Map& dst, const Map& src) {
    for (const auto& tuple : src) {
      dst[tuple.first] = tuple.second;
    }
    return dst;
  }

  inline Map parseINI (String source) {
    Vector<String> entries = split(source, '\n');
    String prefix = "";
    Map settings = {};

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

        prefix = replace(prefix, "\\.", "_");
        if (prefix.size() > 0) {
          prefix += "_";
        }

        continue;
      }

      auto index = entry.find_first_of('=');

      if (index >= 0 && index <= entry.size()) {
        auto key = trim(prefix + entry.substr(0, index));
        auto value = trim(entry.substr(index + 1));

        // trim quotes from quoted strings
        if (value[0] == '"' && value[value.length() - 1] == '"') {
          value = trim(value.substr(1, value.length() - 2));
        }

        auto i = value.find_first_of(';');
        auto j = value.find_first_of('#');

        if (i > 0) {
          value = trim(value.substr(0, i));
        } else if (j > 0) {
          value = trim(value.substr(0, j));
        }

        if (key.ends_with("[]")) {
          key = trim(key.substr(0, key.size() - 2));
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

  //
  // IPC Message parser for the middle end
  // TODO possibly harden data validation.
  //
  class Parse {
    Map args;
    public:
      Parse(const String&);
      int index = -1;
      String value = "";
      String name = "";
      String uri = "";
      String get(const String&) const;
      String get(const String&, const String) const;
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
  inline Parse::Parse (const String& s) {
    String str = s;
    uri = str;

    // bail if missing protocol prefix
    if (str.find("ipc://") == -1) return;

    // bail if malformed
    if (str.compare("ipc://") == 0) return;
    if (str.compare("ipc://?") == 0) return;

    String query;
    String path;

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

  inline String Parse::get(const String& s) const {
    return args.count(s) ? args.at(s): "";
  }

  inline String Parse::get(const String& s, const String fallback) const {
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

  inline std::string stringToHex (const std::string& input) {
    static const char set[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);

    for (unsigned char c : input) {
      output.push_back(set[c >> 4]);
      output.push_back(set[c & 15]);
    }

    return output;
  }

  inline std::string hexToString (const std::string& input) {
    const auto len = input.length();

    std::string output;
    output.reserve(len / 2);

    for (auto it = input.begin(); it != input.end();) {
      int hi = HEX2DEC[(unsigned char) *it++];
      int lo = HEX2DEC[(unsigned char) *it++];
      output.push_back(hi << 4 | lo);
    }

    return output;
  }

  inline String decodeURIComponent (const String& sSrc) {

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

    String sResult(pStart, pEnd);
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

  inline String encodeURIComponent (const String& sSrc) {
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

    String sResult((char*) pStart, (char*) pEnd);
    delete [] pStart;
    return sResult;
  }

  inline auto toBytes (uint64_t n) {
    std::array<uint8_t, 8> bytes;
    // big endian, network order
    bytes[0] = n >> 8*7;
    bytes[1] = n >> 8*6;
    bytes[2] = n >> 8*5;
    bytes[3] = n >> 8*4;
    bytes[4] = n >> 8*3;
    bytes[5] = n >> 8*2;
    bytes[6] = n >> 8*1;
    bytes[7] = n >> 8*0;
    return bytes;
  }

  inline auto msleep (auto n) {
    std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(n));
  }

  inline Vector<String> parseStringList (const String& string, Vector<char> separators) {
    auto list = Vector<String>();
    for (const auto& separator : separators) {
      for (const auto& part: split(string, separator)) {
        list.push_back(part);
      }
    }

    return list;
  }

  inline Vector<String> parseStringList (const String& string, const char separator) {
    return split(string, separator);
  }

  inline Vector<String> parseStringList (const String& string) {
    return parseStringList(string, { ' ', ',' });
  }
}

#endif // SSC_H
