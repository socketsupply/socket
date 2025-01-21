#ifndef SOCKET_RUNTIME_PLATFORM_TYPES_H
#define SOCKET_RUNTIME_PLATFORM_TYPES_H

#include <array>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <ostream>
#include <queue>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <semaphore>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// ambient
namespace ssc {
  namespace fs = std::filesystem;
}

namespace ssc::runtime::types {
  using AtomicBool = std::atomic<bool>;
  using AtomicInt = std::atomic<int>;
  using BinarySemaphore = std::binary_semaphore;
  using ConditionVariable = std::condition_variable;
  using ConditionVariableAny = std::condition_variable_any;
  using String = std::string;
  using StringStream = std::stringstream;
  using WString = std::wstring;
  using WStringStream = std::wstringstream;
  using InputFileStream = std::ifstream;
  using OutputFileStream = std::ofstream;
  using Mutex = std::recursive_mutex;
  using Lock = std::lock_guard<Mutex>;
  using UniqueLock = std::unique_lock<Mutex>;
  using ScopedLock = std::scoped_lock<Mutex, Mutex>;
  using Path = fs::path;
  using Thread = std::thread;
  using Exception = std::exception;
  using Error = std::runtime_error;

  template <typename T> using Atomic = std::atomic<T>;
  template <typename T, int k> using Array = std::array<T, k>;
  template <typename T> using Queue = std::queue<T>;
  template <typename K = String, typename V = String> using Map = std::map<K, V>;
  template <typename K = String, typename V = String> using UnorderedMap = std::unordered_map<K, V>;
  template <typename T> using Vector = std::vector<T>;
  template <typename T> using Function = std::function<T>;
  template <typename X, typename Y> using Tuple = std::tuple<X, Y>;
  template <typename T = String> using Set = std::set<T>;
  template <typename T = String> using UnorderedSet = std::unordered_set<T>;
  template <typename T> using SharedPointer = std::shared_ptr<T>;
  template <typename T> using UniquePointer = std::unique_ptr<T>;
  template <typename T> using Promise = std::promise<T>;
  template <typename T> using InputStreamBufferIterator = std::istreambuf_iterator<T>;
  template <std::ptrdiff_t k> using Semaphore = std::counting_semaphore<k>;

  using ExitCallback = Function<void(int code)>;
  using MessageCallback = Function<void(const String)>;
}
#endif
