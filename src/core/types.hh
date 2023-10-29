#ifndef SSC_CORE_TYPES_H
#define SSC_CORE_TYPES_H

#include <array>
#include <atomic>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace SSC {
  using std::format;

#if !defined(__APPLE__) || (defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
  namespace fs = std::filesystem;
  using Path = fs::path;
#endif

  using AtomicBool = std::atomic<bool>;
  using String = std::string;
  using StringStream = std::stringstream;
  using WString = std::wstring;
  using WStringStream = std::wstringstream;
  using Map = std::map<String, String>;
  using Mutex = std::recursive_mutex;
  using Lock = std::lock_guard<Mutex>;
  using Thread = std::thread;
  using Exception = std::exception;
  using InputFileStream = std::fstream;

  template <typename T> using Atomic = std::atomic<T>;
  template <typename T, int k> using Array = std::array<T, k>;
  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;
  template <typename T> using Set = std::unordered_set<T>;
  template <typename T> using OrderedSet = std::set<T>;
  template <typename T> using Function = std::function<T>;

  using ExitCallback = std::function<void(int code)>;
  using MessageCallback = std::function<void(const String)>;
}

#endif
