#ifndef CORE_PRELOAD_HH
#define CORE_PRELOAD_HH

#include "../window/options.hh"

namespace SSC {
  struct PreloadOptions {
    bool module = false;
    bool wrap = false;
    String userScript = "";
  };

  String createPreload (
    const WindowOptions opts,
    const PreloadOptions preloadOptions
  );

  inline SSC::String createPreload (WindowOptions opts) {
    return createPreload(opts, PreloadOptions {});
  }
}
#endif
