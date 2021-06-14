#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include "util.h"

inline std::string getCwd(const std::string) {
  return pathToString(fs::current_path());
}

inline std::wstring getExec() {
  TCHAR buffer[MAX_PATH] = { 0 };
  DWORD len = GetModuleFileName(NULL, buffer, MAX_PATH);
  std::wstring ws;
  ws.assign(&buffer[0], &buffer[len]);
  std::wstring::size_type pos = ws.find_last_of(L"\\/");
  return ws.substr(0, pos);
}