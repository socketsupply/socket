#ifndef OPKIT_H
#define OPKIT_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <regex>
#include <fstream>
#include <span>
#include <thread>
#include <filesystem>

#include "preload.hh"

#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

//
// A cross platform MAIN macro that
// magically gives us argc and argv.
//
#if defined(_WIN32)
#define MAIN \
  int argc = __argc; \
  char** argv = __argv; \
  \
  int CALLBACK WinMain(\
    _In_ HINSTANCE hInstance,\
    _In_ HINSTANCE hPrevInstance,\
    _In_ LPSTR lpCmdLine,\
    _In_ int nCmdShow)

#else
#define MAIN int main (int argc, char** argv)
#endif

namespace fs = std::filesystem;

enum {
  NOC_FILE_DIALOG_OPEN    = 1 << 0,   // Create an open file dialog.
  NOC_FILE_DIALOG_SAVE    = 1 << 1,   // Create a save file dialog.
  NOC_FILE_DIALOG_DIR     = 1 << 2,   // Open a directory.
  NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION = 1 << 3,
};

namespace Opkit {
  //
  // Cross platform support for strings
  //
  #if defined(_WIN32)
    using String = std::wstring;
    using Stringstream = std::wstringstream;
    using namespace Microsoft::WRL;
    #define Str(s) L##s

  #else
    using String = std::string;
    using Stringstream = std::stringstream;
    #define Str(s) s

  #endif

  //
  // Reporting on the platform (for the cli).
  //
  struct {
    #if defined(_WIN32)
      bool darwin = false;
      bool win = true;
      bool linux = false;
      const String os = "win";

    #elif defined(__APPLE__)
      bool darwin = true;
      bool win = false;
      bool linux = false;
      const String os = "darwin";

    #elif defined(__linux__)
      bool darwin = false;
      bool win = false;
      bool linux = true;
      const String os = "linux";

    #endif
  } platform;

  //
  // Application data
  //
  std::map<std::string, std::string> appData;

  //
  // Window data
  //
  struct WindowOptions {
    bool resizable = true;
    bool frameless = false;
    int height;
    int width;
    String title = Str("");
    String url = Str("data:text/html,<html>");
    String preload = Str("");
  };

  //
  // Helper functions...
  //
  inline const std::vector<String>
  split(const String& s, const char& c) {
    String buff;
    std::vector<String> vec;
    
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

  inline String&
  trim(String& str) {
    str.erase(0, str.find_first_not_of(" \r\n\t"));
    str.erase(str.find_last_not_of(" \r\n\t") + 1);
    return str;
  }

  inline String tmpl(const String s, std::map<String, String> pairs) {
    String output = s;

    for (auto item : pairs) {
      auto key = "[{]+(" + item.first + ")[}]+";
      auto value = item.second;
      output = std::regex_replace(output, std::regex(key), value);
    }

    return output;
  }

  inline String replace(String src, String re, String val) {
    return std::regex_replace(src, std::regex(re), val);
  }

  inline auto getEnv(String variableName) {
    #if _WIN32
      char* variableValue = nullptr;
      std::size_t valueSize = 0;
      auto query = _dupenv_s(&variableValue, &valueSize, variableName.c_str());

      String result;
      if(query == 0 && variableValue != nullptr && valueSize > 0) {
        result.assign(variableValue, valueSize - 1);
        free(variableValue);
      }

      return result;
    #else
      auto v = getenv(variableName.c_str());

      if (v != nullptr) {
        return String(v);
      }

      return String("");
    #endif
  }

  inline auto setEnv(String s) {
    #if _WIN32
      return _putenv(s.c_str());
    #else
      return putenv((char*) s.c_str());
    #endif
  }

  inline String exec(String command) {
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
      std::cout << "error: unable to open the command" << std::endl;
      exit(1);
    }

    Stringstream ss;

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

  inline String pathToString(const fs::path &path) {
    auto s = path.u8string();
    return String(s.begin(), s.end());
  }

  inline String readFile(fs::path path) {
    std::ifstream stream(path.c_str());
    String content;
    auto buffer = std::istreambuf_iterator<char>(stream);
    auto end = std::istreambuf_iterator<char>();
    content.assign(buffer, end);
    stream.close();
    return content;
  }

  inline void writeFile (fs::path path, String s) {
    std::ofstream stream(pathToString(path));
    stream << s;
    stream.close();
  }

  inline String prefixFile(String s) {
    if (platform.darwin || platform.linux) {
      return Str("/usr/local/lib/opkit/" + s + " ");
    }

    String local = getEnv("LOCALAPPDATA");
    return Str(local + "\\Programs\\optoolco\\" + s + " ");
  }

  inline String prefixFile() {
    if (platform.darwin || platform.linux) {
      return Str("/usr/local/lib/opkit");
    }

    String local = getEnv("LOCALAPPDATA");
    return Str(local + "\\Programs\\optoolco");
  }

  inline std::map<String, String> parseConfig(String source) {
    auto entries = split(source, '\n');
    std::map<String, String> settings;

    for (auto entry : entries) {
      auto pair = split(entry, ':');

      if (pair.size() == 2) {
        settings[trim(pair[0])] = trim(pair[1]);
      }
    }

    return settings;
  }

  //
  // IPC Message parser for the middle end
  // TODO possibly harden data validation.
  //
  struct Parse {
    Parse(String);
    int index = 0;
    String value = Str("");
    String name = Str("");
    std::map<String, String> args;
  };

  //
  // cmd: `ipc://id?p1=v1&p2=v2&...\0`
  //
  inline Parse::Parse(String str) {    
    if (str.find("ipc://") == -1) return;

    String query;
    String path;

    auto raw = split(str, '?');
    path = raw[0];
    query = raw[1];

    auto parts = split(path, '/');
    name = parts[1];

    if (raw.size() != 2) return;
    auto pairs = split(raw[1], '&');

    for (auto& rawPair : pairs) {
      auto pair = split(rawPair, '=');
      if (pair.size() != 2) continue;
      args[pair[0]] = pair[1];
    }
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

  inline String decodeURIComponent(const String& sSrc) {

    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    auto s = replace(String(sSrc), "\\+", " ");
    const unsigned char * pSrc = (const unsigned char *) s.c_str();
    const int SRC_LEN = sSrc.length();
    const unsigned char * const SRC_END = pSrc + SRC_LEN;
    const unsigned char * const SRC_LAST_DEC = SRC_END - 2;   // last decodable '%' 

    char * const pStart = new char[SRC_LEN];
    char * pEnd = pStart;

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

  //
  // Interfaces make sure all operating systems implement the same stuff
  //
  class IApp {
    public:
      bool shouldExit = false;

      virtual int run() = 0;
      virtual void exit() = 0;
      virtual void dispatch(std::function<void()> work) = 0;
      virtual String getCwd(String) = 0;
  };

  class IWindow {
    public:
      String resolve(String, String, String);
      String emit(String, String);

      String url;
      String title;
      bool initDone = false;
      std::function<void(String)> _onMessage = nullptr;

      virtual void onMessage(std::function<void(String)>) = 0;
      virtual void eval(const String&) = 0;
      virtual void show() = 0;
      virtual void hide() = 0 ;
      virtual void navigate(const String&) = 0;
      virtual void setTitle(const String&) = 0;
      virtual void setContextMenu(String, String) = 0;
      virtual String openDialog(bool, bool, bool, String, String) = 0;
      virtual void setSystemMenu(String menu) = 0;
      virtual int openExternal(String s) = 0;
  };

  String IWindow::resolve(String seq, String status, String value) {
    String response = Str(
      "(() => {"
      "  const seq = Number(" + seq + ");"
      "  const status = Number(" + status + ");"
      "  const value = '" + value + "';"
      "  window._ipc.resolve(seq, status, value);"
      "})()"
    );

    return response;
  }

  String IWindow::emit(String event, String value) {
    String response = Str(
      "(() => {"
      "  const name = '" + event + "';"
      "  const value = '" + value + "';"
      "  window._ipc.emit(name, value);"
      "})()"
    );

    return response;
  }
}

#endif // OPKIT_H
