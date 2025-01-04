#ifndef SOCKET_APP_H
#define SOCKET_APP_H

#include "runtime.hh"

namespace ssc::app {
  class App : public runtime::app::App {
    public:
      using runtime::app::App::App;
  };
}
#endif
