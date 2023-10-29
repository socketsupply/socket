#if !defined(_WIN32)
#include <unistd.h>
#else
#include <array>
#include <AppxPackaging.h>
#include <comdef.h>
#include <functional>
#include <shlwapi.h>
#include <strsafe.h>
#include <tchar.h>
#include <wrl.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "msvcrt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include "cli.hh"

SSC::CLI::Program program;
SSC::UserConfig userConfig;

const SSC::Config& SSC::getUserConfig () {
  return program.userConfig;
}

bool SSC::isDebugEnabled () {
  return program.state.program.debug;
}

int main (const int argc, const char** argv) {
  return program.main(argc, argv);
}
