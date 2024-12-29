#ifndef SOCKET_RUNTIME_OPTIONS_H
#define SOCKET_RUNTIME_OPTIONS_H

#include "platform.hh"

namespace ssc::runtime {
  struct Options {
    template <typename T> const T& as () const {
      static_assert(std::is_base_of<Options, T>::value);
      return *reinterpret_cast<const T*>(this);
    }
  };
}
#endif
