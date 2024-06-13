#ifndef SOCKET_RUNTIME_PLATFORM_TYPES_H
#define SOCKET_RUNTIME_PLATFORM_TYPES_H

#include <array>
#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace SSC {
  namespace fs = std::filesystem;

  using AtomicBool = std::atomic<bool>;
  using AtomicInt = std::atomic<int>;
  using String = std::string;
  using StringStream = std::stringstream;
  using WString = std::wstring;
  using WStringStream = std::wstringstream;
  using Mutex = std::recursive_mutex;
  using Lock = std::lock_guard<Mutex>;
  using Map = std::map<String, String>;
  using Path = fs::path;
  using Thread = std::thread;
  using Exception = std::exception;

  template <typename T> using Atomic = std::atomic<T>;
  template <typename T, int k> using Array = std::array<T, k>;
  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;
  template <typename T> using Function = std::function<T>;
  template <typename X, typename Y> using Tuple = std::tuple<X, Y>;
  template <typename T = String> using Set = std::set<T>;
  template <typename T> using SharedPointer = std::shared_ptr<T>;
  template <typename T> using UniquePointer = std::unique_ptr<T>;

  using ExitCallback = Function<void(int code)>;
  using MessageCallback = Function<void(const String)>;
}

#endif
