#ifndef SOCKET_RUNTIME_CORE_OPTIONS_H
#define SOCKET_RUNTIME_CORE_OPTIONS_H

#include "../platform/types.hh"

namespace SSC {
  struct Options {
    template <typename T> const T& as () const {
      static_assert(std::is_base_of<Options, T>::value);
      return *reinterpret_cast<const T*>(this);
    }
  };
}
#endif
