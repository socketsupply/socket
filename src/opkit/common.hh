#ifndef OPKIT_H
#define OPKIT_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

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

namespace Opkit {
  //
  // Cross platform support for strings
  //
  #if defined(_WIN32)
    using String = std::wstring;
    using namespace Microsoft::WRL;
    #define Str(s) L##s

  #else
    using String = std::string;
    #define Str(s) s

  #endif

  //
  // Reporting on the platform (for the cli).
  //
  struct {
    #if defined(_WIN32)
      bool darwin = false;
      bool win32 = true;
      bool linux = false;
      const String os = "win32";

    #elif defined(__APPLE__)
      bool darwin = true;
      bool win32 = false;
      bool linux = false;
      const String os = "darwin";

    #elif defined(__linux__)
      bool darwin = false;
      bool win32 = false;
      bool linux = true;
      const String os = "linux";

    #endif
  } platform;

  //
  // Window
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

  inline String pathToString(const fs::path &path) {
    auto s = path.u8string();
    return std::string(s.begin(), s.end());
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
    
    const unsigned char * pSrc = (const unsigned char *)sSrc.c_str();
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
}

#endif // OPKIT_H
