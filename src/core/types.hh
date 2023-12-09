#ifndef SSC_CORE_TYPES_H
#define SSC_CORE_TYPES_H

#include <array>
#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace SSC {
  namespace fs = std::filesystem;

  using AtomicBool = std::atomic<bool>;
  using String = std::string;
  using StringStream = std::stringstream;
  using WString = std::wstring;
  using WStringStream = std::wstringstream;
  using Map = std::map<String, String>;
  using Mutex = std::recursive_mutex;
  using Path = fs::path;
  using Lock = std::lock_guard<Mutex>;
  using Thread = std::thread;
  using Exception = std::exception;

  template <typename T> using Atomic = std::atomic<T>;
  template <typename T, int k> using Array = std::array<T, k>;
  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;

  using ExitCallback = std::function<void(int code)>;
  using MessageCallback = std::function<void(const String)>;
}

#endif
