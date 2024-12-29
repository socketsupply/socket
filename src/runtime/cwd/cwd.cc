#include "../filesystem.hh"

#include "../cwd.hh"

namespace ssc::runtime {
  static struct { Mutex mutex; String value = ""; } state;

  void setcwd (const String& value) {
    Lock lock(state.mutex);
    state.value = value;
  }

  const String getcwd_state_value () {
    Lock lock(state.mutex);
    return state.value;
  }

  const String getcwd () {
    Lock lock(state.mutex);

    if (state.value.size() > 0) {
      return state.value;
    }

    state.value = runtime::filesystem::Resource::getResourcesPath().string();
    return state.value;
  }
}
