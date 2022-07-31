#ifndef SSC_H
#define SSC_H

#include "preload.hh"

// TODO could be conditional based on not (_WIN32, IOS, ANDROID)

#include <string>
#include <vector>
#include <map>
#include <any>
#include <iostream>
#include <sstream>
#include <regex>
#include <fstream>
#include <span>
#include <filesystem>

#if defined(_WIN32)
#include <Windows.h>
#include <tchar.h>
#include <wrl.h>
#include <functional>
#include <array>

//
// A cross platform MAIN macro that
// magically gives us argc and argv.
//

#define MAIN \
  int argc = __argc; \
  char** argv = __argv; \
  \
  int CALLBACK WinMain(\
    _In_ HINSTANCE instanceId,\
    _In_ HINSTANCE hPrevInstance,\
    _In_ LPSTR lpCmdLine,\
    _In_ int nCmdShow)

#else
#define MAIN \
  int instanceId = 0; \
  int main (int argc, char** argv)
#endif

#ifndef WIFEXITED
#define WIFEXITED(w) ((w) & 0x7f)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(w) (((w) & 0xff00) >> 8)
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SOCKET_MAX_WINDOWS
#define SOCKET_MAX_WINDOWS 32
#endif

#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

namespace fs = std::filesystem;
using Map = std::map<std::string, std::string>;
using SCallback = std::function<void(const std::string)>;
using ExitCallback = std::function<void(int code)>;

namespace SSC {
  constexpr auto version_hash = STR_VALUE(VERSION_HASH);
  constexpr auto version = STR_VALUE(VERSION);
  constexpr auto full_version = STR_VALUE(VERSION) " (" STR_VALUE(VERSION_HASH) ")";

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

  //
  // Cross platform support for strings
  //
  #if defined(_WIN32)
    using String = std::wstring;
    using Stringstream = std::wstringstream;
    using namespace Microsoft::WRL;
    #define Str(s) L##s
    #define RegExp std::wregex

    inline std::wstring StringToWString(const std::string& s) {
      std::wstring temp(s.length(), L' ');
      std::copy(s.begin(), s.end(), temp.begin());
      return temp;
    }

    inline std::string WStringToString(const std::wstring& s) {
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
  // Reporting on the platform (for the cli).
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
      bool win = true;
      bool linux = false;
      bool unix = false;

    #elif defined(__APPLE__)
      std::string os = "mac";
      bool mac = true;
      bool win = false;
      bool linux = false;

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
      bool win = false;
      bool linux = false;

      #if defined(__unix__) || defined(unix) || defined(__unix)
      bool unix = true;
      #else
      bool unix = false;
      #endif

    #elif defined(BSD)
      const std::string os = "openbsd";
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

  //
  // Application data
  //
  Map appData;

  //
  // Window data
  //
  struct WindowOptions {
    bool resizable = true;
    bool frameless = false;
    bool utility = false;
    bool canExit = false;
    int height = 0;
    int width = 0;
    int index = 0;
    int debug = 0;
    int port = 0;
    bool isTest = false;
    bool headless = false;
    bool forwardConsole = false;
    std::string cwd = "";
    std::string executable = "";
    std::string title = "";
    std::string url = "data:text/html,<html>";
    std::string version = "";
    std::string argv = "";
    std::string preload = "";
    std::string env;
    SCallback onMessage = [](const std::string) {};
    ExitCallback onExit = nullptr;
  };

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

  inline std::string replace(const std::string& src, const std::string& re, const std::string& val) {
    return std::regex_replace(src, std::regex(re), val);
  }

  inline std::string& replaceAll(std::string& src, std::string const& from, std::string const& to) {
    size_t start = 0;
    size_t index;

    while ((index = src.find(from, start)) != std::string::npos) {
      src.replace(index, from.size(), to);
      start = index + to.size();
    }
    return src;
  }

  std::string gMobilePreload = "";

  std::string createPreload(WindowOptions opts) {
    std::string cleanCwd = std::string(opts.cwd);
    std::replace(cleanCwd.begin(), cleanCwd.end(), '\\', '/');

    auto preload = std::string(
      "(() => {"
      "  window.system = {};\n"
      "  window.process = {};\n"
      "  window.process.index = Number(`" + std::to_string(opts.index) + "`);\n"
      "  window.process.port = Number(`" + std::to_string(opts.port) + "`);\n"
      "  window.process.cwd = () => `" + cleanCwd + "`;\n"
      "  window.process.title = `" + opts.title + "`;\n"
      "  window.process.executable = `" + opts.executable + "`;\n"
      "  window.process.version = `" + opts.version + "`;\n"
      "  window.process.debug = " + std::to_string(opts.debug) + ";\n"
      "  window.process.platform = `" + platform.os + "`;\n"
      "  window.process.arch = `" + platform.arch + "`;\n"
      "  window.process.env = Object.fromEntries(new URLSearchParams(`" +  opts.env + "`));\n"
      "  window.process.openFds = new Map();\n"
      "  window.process.config = {};\n"
      "  window.process.argv = [" + opts.argv + "];\n"
      "  " + gPreload + "\n"
      "  " + opts.preload + "\n"
    );

    if (opts.headless) {
      preload += "                                                  \n"
        "console.log = (...args) => {                               \n"
        "  const { index } = window.process;                        \n"
        "  const value = args                                       \n"
        "    .map(encodeURIComponent)                               \n"
        "    .join('');                                             \n"
        "  const uri = `ipc://log?index=${index}&value=${value}`;   \n"
        "  window.external.invoke(uri);                             \n"
        "};                                                         \n"
        "console.warn = console.error = console.log;                \n";
    }

    for (auto const &tuple : appData) {
      auto key = tuple.first;
      auto value = tuple.second;
      preload += "  window.process.config['" + key  + "'] = '" + value + "';\n";
    }

    preload += "  Object.seal(Object.freeze(window.process.config));\n";
    preload += "})()\n";
    preload += "//# sourceURL=preload.js";
    return preload;
  }

  std::string emitToRenderProcess(const std::string& event, const std::string& value) {
    return std::string(
      "(() => {"
      "  const name = `" + event + "`;"
      "  const value = `" + value + "`;"
      "  window._ipc.emit(name, value);"
      "})()"
    );
  }

  std::string streamToRenderProcess(const std::string& id, const std::string& value) {
    return std::string(
      "(() => {"
      "  const id = `" + id + "`;"
      "  const value = `" + value + "`;"
      "  window._ipc.callbacks[id] && window._ipc.callbacks[id](null, value);"
      "})()"
    );
  }

  std::string resolveMenuSelection(const std::string& seq, const std::string& title, const std::string& parent) {
    return std::string(
      "(() => {"
      "  const detail = {"
      "    title: `" + title + "`,"
      "    parent: `" + parent + "`,"
      "    state: '0'"
      "  };"

      "  if (" + seq + " > 0 && window._ipc[" + seq + "]) {"
      "    window._ipc[" + seq + "].resolve(detail);"
      "    delete window._ipc[" + seq + "];"
      "    return;"
      "  }"

      "  const event = new window.CustomEvent('menuItemSelected', { detail });"
      "  window.dispatchEvent(event);"
      "})()"
    );
  }

  //
  // Helper functions...
  //
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

  inline std::string
  trim(std::string str) {
    str.erase(0, str.find_first_not_of(" \r\n\t"));
    str.erase(str.find_last_not_of(" \r\n\t") + 1);
    return str;
  }

  inline std::string tmpl(const std::string s, Map pairs) {
    std::string output = s;

    for (auto item : pairs) {
      auto key = std::string("[{]+(" + item.first + ")[}]+");
      auto value = item.second;
      output = std::regex_replace(output, std::regex(key), value);
    }

    return output;
  }

  uint64_t rand64(void) {
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

  struct ExecOutput {
    std::string output;
    int exitCode = 0;
  };

  inline ExecOutput exec(std::string command) {
    command = command + " 2>&1";

    ExecOutput eo;
    FILE* pipe;
    size_t count;
    int exitCode = 0;
    const int bufsize = 128;
    std::array<char, 128> buffer;

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
      std::cout << "error: unable to open the command" << std::endl;
      exit(1);
    }

    do {
      if ((count = fread(buffer.data(), 1, bufsize, pipe)) > 0) {
        eo.output.insert(eo.output.end(), std::begin(buffer), std::next(std::begin(buffer), count));
      }
    } while (count > 0);

    #ifdef _WIN32
    exitCode = _pclose(pipe);
    #else
    exitCode = pclose(pipe);
    #endif

    if (!WIFEXITED(exitCode) || exitCode != 0) {
      auto status = WEXITSTATUS(exitCode);
      if (status && exitCode) {
        exitCode = status;
      }
    }

    eo.exitCode = exitCode;

    return eo;
  }

  inline void writeToStdout(const std::string &str) {
    #ifdef _WIN32
      std::stringstream ss;
      ss << str << std::endl;
      auto lineStr = ss.str();

      WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), lineStr.c_str(), lineStr.size(), NULL, NULL);
    #else
      std::cout << str << std::endl;
    #endif
  }

  #if IOS == 0 && !defined(TARGET_OS_SIMULATOR)
    inline String readFile(fs::path path) {
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

  inline std::string prefixFile(std::string s) {
    if (platform.mac || platform.linux) {
      std::string local = getEnv("HOME");
      return std::string(local + "/.config/socket-sdk/" + s + " ");
    }

    std::string local = getEnv("LOCALAPPDATA");
    return std::string(local + "\\Programs\\socketsupply\\" + s + " ");
  }

  inline std::string prefixFile() {
    if (platform.mac || platform.linux) {
      std::string local = getEnv("HOME");
      return std::string(local + "/.config/socket-sdk/");
    }

    std::string local = getEnv("LOCALAPPDATA");
    return std::string(local + "\\Programs\\socketsupply");
  }

  inline Map parseConfig(std::string source) {
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
  };

  struct ScreenSize {
    int height = 0;
    int width = 0;
  };

  //
  // cmd: `ipc://id?p1=v1&p2=v2&...\0`
  //
  inline Parse::Parse(const std::string& s) {
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

  std::string Parse::get(const std::string& s) const {
    return args.count(s) ? args.at(s): "";
  }

  std::string Parse::get(const std::string& s, const std::string fallback) const {
    return args.count(s) ? args.at(s) : fallback;
  }

  //
  // All ipc uses a URI schema, so all ipc data needs to be
  // encoded as a URI component. This prevents escaping the
  // protocol.
  //
  const char HEX2DEC[256] = {
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

  const char SAFE[256] = {
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

  static inline std::string encodeURIComponent(const std::string& sSrc) {
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

  //
  // Interfaces make sure all operating systems implement the same stuff
  //
  class IApp {
    // an opaque pointer to the configured `WindowFactory<Window, App>`
    void *windowFactory = nullptr;

    public:
      bool shouldExit = false;
      bool fromSSC = false;
      ExitCallback onExit = nullptr;
      void exit(int code);
      void setWindowFactory(void *windowFactory) {
        this->windowFactory = windowFactory;
      }

      void * getWindowFactory() {
        return this->windowFactory;
      }

      virtual int run() = 0;
      virtual void kill() = 0;
      virtual void restart() = 0;
      virtual void dispatch(std::function<void()> work) = 0;
      virtual std::string getCwd(const std::string&) = 0;
  };

  void IApp::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  class Window;

  class IWindow {
    public:
      int index = 0;
      void* bridge;
      WindowOptions opts;
      SCallback onMessage = [](const std::string) {};
      ExitCallback onExit = nullptr;
      void resolvePromise (const std::string& seq, const std::string& state, const std::string& value);

      virtual void eval(const std::string&) = 0;
      virtual void show(const std::string&) = 0;
      virtual void hide(const std::string&) = 0;
      virtual void close(int code) = 0;
      virtual void exit(int code) = 0;
      virtual void kill() = 0;
      virtual void navigate(const std::string&, const std::string&) = 0;
      virtual void setSize(const std::string&, int, int, int) = 0;
      virtual void setTitle(const std::string&, const std::string&) = 0;
      virtual void setContextMenu(const std::string&, const std::string&) = 0;
      virtual void setSystemMenu(const std::string&, const std::string&) = 0;
      virtual void setSystemMenuItemEnabled(bool enabled, int barPos, int menuPos) = 0;
      virtual void openDialog(const std::string&, bool, bool, bool, bool, const std::string&, const std::string&, const std::string&) = 0;
      virtual void setBackgroundColor(int r, int g, int b, float a) = 0;
      virtual void showInspector() = 0;
      virtual ScreenSize getScreenSize() = 0;
  };

  std::string resolveToRenderProcess(const std::string& seq, const std::string& state, const std::string& value) {
    return std::string(
      "(() => {"
      "  const seq = String(`" + seq + "`);"
      "  const state = Number(`" + state + "`);"
      "  const value = `" + value + "`;"
      "  window._ipc.resolve(seq, state, value);"
      "})()"
    );
  }

  std::string resolveToMainProcess(const std::string& seq, const std::string& state, const std::string& value) {
    return std::string("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }

  void IWindow::resolvePromise (const std::string& seq, const std::string& state, const std::string& value) {
    if (seq.find("R") == 0) {
      this->eval(resolveToRenderProcess(seq, state, value));
    }
    this->onMessage(resolveToMainProcess(seq, state, value));
  }

  struct WindowFactoryOptions {
    int defaultHeight = 0;
    int defaultWidth = 0;
    bool headless = false;
    bool isTest;
    std::string argv = "";
    std::string cwd = "";
    SCallback onMessage = [](const std::string) {};
    ExitCallback onExit = nullptr;
  };

  template <class Window> class IWindowFactory {
    public:
      WindowFactoryOptions options;

      virtual void destroy () = 0;
      virtual void configure (WindowFactoryOptions) = 0;
      virtual Window* getWindow (int) = 0;
      virtual void destroyWindow (int) = 0;
      virtual void destroyWindow (Window*) = 0;
      virtual Window* createWindow (WindowOptions) = 0;
      virtual Window* createDefaultWindow (WindowOptions) = 0;
  };

  template <class Window, class App> class WindowFactory : public IWindowFactory<Window> {
    public:
      enum WindowStatus {
        WINDOW_ERROR = -1,
        WINDOW_NONE = 0,
        WINDOW_CREATING = 10,
        WINDOW_CREATED,
        WINDOW_HIDING = 20,
        WINDOW_HIDDEN,
        WINDOW_SHOWING = 30,
        WINDOW_SHOWN,
        WINDOW_CLOSING = 40,
        WINDOW_CLOSED,
        WINDOW_EXITING = 50,
        WINDOW_EXITED,
        WINDOW_KILLING = 60,
        WINDOW_KILLED
      };

      class WindowWithMetadata : public Window {
        public:
          WindowStatus status;
          WindowFactory<Window, App> factory;

          WindowWithMetadata (
            WindowFactory &factory,
            App &app,
            WindowOptions opts
          ) : Window(app, opts)
            , factory(factory)
          {
            // noop
          }

          ~WindowWithMetadata () {}

          void show (const std::string &seq) {
            auto index = std::to_string(this->opts.index);
            factory.debug("Showing Window#" + index + " (seq=" + seq + ")");
            status = WindowStatus::WINDOW_SHOWING;
            Window::show(seq);
            status = WindowStatus::WINDOW_SHOWN;
          }

          void hide (const std::string &seq) {
            if (
              status > WindowStatus::WINDOW_HIDDEN &&
              status < WindowStatus::WINDOW_EXITING
            ) {
              auto index = std::to_string(this->opts.index);
              factory.debug("Hiding Window#" + index + " (seq=" + seq + ")");
              status = WindowStatus::WINDOW_HIDING;
              Window::hide(seq);
              status = WindowStatus::WINDOW_HIDDEN;
            }
          }

          void close (int code) {
            if (status < WindowStatus::WINDOW_CLOSING) {
              auto index = std::to_string(this->opts.index);
              factory.debug("Closing Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_CLOSING;
              Window::close(code);
              status = WindowStatus::WINDOW_CLOSED;
              // gc();
            }
          }

          void exit (int code) {
            if (status < WindowStatus::WINDOW_EXITING) {
              auto index = std::to_string(this->opts.index);
              factory.debug("Exiting Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_EXITING;
              Window::exit(code);
              status = WindowStatus::WINDOW_EXITED;
              gc();
            }
          }

          void kill () {
            if (status < WindowStatus::WINDOW_KILLING) {
              auto index = std::to_string(this->opts.index);
              factory.debug("Killing Window#" + index);
              status = WindowStatus::WINDOW_KILLING;
              Window::kill();
              status = WindowStatus::WINDOW_KILLED;
              gc();
            }
          }

          void gc () {
            factory.destroyWindow(reinterpret_cast<Window*>(this));
          }
      };

#if DEBUG
      std::chrono::system_clock::time_point lastDebugLogLine;
#endif

      App app;
      bool destroyed = false;
      std::vector<bool> inits;
      std::vector<WindowWithMetadata*> windows;

      WindowFactory (App &app) :
        app(app),
        inits(SOCKET_MAX_WINDOWS),
        windows(SOCKET_MAX_WINDOWS)
    {
#if DEBUG
        lastDebugLogLine = std::chrono::system_clock::now();
#endif
      }

      ~WindowFactory () {
        destroy();
      }

      void destroy () {
        if (this->destroyed) return;
        for (auto window : windows) {
          destroyWindow(window);
        }

        this->destroyed = true;

        windows.clear();
        inits.clear();
      }

      void configure (WindowFactoryOptions configuration) {
        if (destroyed) return;
        this->options.defaultHeight = configuration.defaultHeight;
        this->options.defaultWidth = configuration.defaultWidth;
        this->options.onMessage = configuration.onMessage;
        this->options.onExit = configuration.onExit;
        this->options.headless = configuration.headless;
        this->options.isTest = configuration.isTest;
        this->options.argv = configuration.argv;
        this->options.cwd = configuration.cwd;
      }

      void inline debug (const std::string line) {
        if (destroyed) return;
#if DEBUG
        using namespace std::chrono;

#ifdef _WIN32 // unicode console support
        // SetConsoleOutputCP(CP_UTF8);
        // setvbuf(stdout, nullptr, _IOFBF, 1000);
#endif

        auto now = system_clock::now();
        auto delta = duration_cast<milliseconds>(now - lastDebugLogLine).count();

        std::cout << "• " << line;
        std::cout << " \033[0;32m+" << delta << "ms\033[0m";
        std::cout << std::endl;

        lastDebugLogLine = now;
#endif
      }

      Window* getWindow (int index, WindowStatus status) {
        if (this->destroyed) return nullptr;
        if (
          getWindowStatus(index) > WindowStatus::WINDOW_NONE &&
          getWindowStatus(index) < status
        ) {
          return reinterpret_cast<Window*>(windows[index]);
        }

        return nullptr;
      }

      Window* getWindow (int index) {
        return getWindow(index, WindowStatus::WINDOW_EXITING);
      }

      Window* getOrCreateWindow (int index) {
        return getOrCreateWindow(index, WindowOptions {});
      }

      Window* getOrCreateWindow (int index, WindowOptions opts) {
        if (this->destroyed) return nullptr;
        if (index < 0) return nullptr;
        if (getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
          opts.index = index;
          return createWindow(opts);
        }

        return getWindow(index);
      }

      WindowStatus getWindowStatus (int index) {
        if (this->destroyed) return WindowStatus::WINDOW_NONE;
        if (index >= 0 && inits[index]) {
          return windows[index]->status;
        }

        return WindowStatus::WINDOW_NONE;
      }

      void destroyWindow (int index) {
        if (destroyed) return;
        if (index >= 0 && inits[index] && windows[index] != nullptr) {
          return destroyWindow(windows[index]);
        }
      }

      void destroyWindow (WindowWithMetadata* window) {
        if (destroyed) return;
        if (window != nullptr) {
          return destroyWindow(reinterpret_cast<Window*>(window));
        }
      }

      void destroyWindow (Window* window) {
        if (destroyed) return;
        if (window != nullptr && windows[window->index] != nullptr) {
          auto metadata = reinterpret_cast<WindowWithMetadata*>(window);

          inits[window->index] = false;
          windows[window->index] = nullptr;

          if (metadata->status < WINDOW_CLOSING) {
            window->close(0);
          }

          if (metadata->status < WINDOW_KILLING) {
            window->kill();
          }

          delete window;
        }
      }

      Window* createWindow (WindowOptions opts) {
        if (destroyed) return nullptr;
        std::stringstream env;

        if (inits[opts.index]) {
          return reinterpret_cast<Window*>(windows[opts.index]);
        }

        for (auto const &envKey : split(appData["env"], ',')) {
          auto cleanKey = trim(envKey);
          auto envValue = getEnv(cleanKey.c_str());

          env << std::string(
            cleanKey + "=" + encodeURIComponent(envValue) + "&"
          );
        }

        auto height = opts.height > 0 ? opts.height : this->options.defaultHeight;
        auto width = opts.width > 0 ? opts.width : this->options.defaultWidth;

        WindowOptions windowOptions = {
          .resizable = opts.resizable,
          .frameless = opts.frameless,
          .utility = opts.utility,
          .canExit = opts.canExit,
          .height = height,
          .width = width,
          .index = opts.index,
#if DEBUG
          .debug = DEBUG || opts.debug,
#else
          .debug = opts.debug,
#endif
          .port = opts.port,
          .isTest = this->options.isTest,
          .headless = this->options.headless || opts.headless || appData["headless"] == "true",
          .forwardConsole = appData["forward_console"] == "true",

          .cwd = this->options.cwd,
          .executable = appData["executable"],
          .title = opts.title.size() > 0 ? opts.title : appData["title"],
          .url = opts.url.size() > 0 ? opts.url : "data:text/html,<html>",
          .version = appData["version"],
          .argv = this->options.argv,
          .preload = opts.preload.size() > 0 ? opts.preload : gPreloadDesktop,
          .env = env.str()
        };

#if DEBUG
        this->debug("Creating Window#" + std::to_string(opts.index));
#endif
        auto window = new WindowWithMetadata(*this, app, windowOptions);

        window->status = WindowStatus::WINDOW_CREATED;
        window->onExit = this->options.onExit;
        window->onMessage = this->options.onMessage;

        windows[opts.index] = window;
        inits[opts.index] = true;

        return reinterpret_cast<Window*>(window);
      }

      Window* createDefaultWindow (WindowOptions opts) {
        return createWindow(WindowOptions {
          .resizable = true,
          .frameless = false,
          .canExit = true,
          .height = opts.height,
          .width = opts.width,
          .index = 0,
#ifdef PORT
          .port = PORT
#endif
        });
      }
  };
}

#endif // SSC_H
