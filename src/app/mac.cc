#include "app.hh"
#include "../core/core.hh"

namespace SSC {
  App::App (int instanceId) {
    // TODO enforce single instance is set
  }

  int App::run () {
    /* NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:true];
    if (event) {
        [NSApp sendEvent:event];
    } */

    auto cwd = getCwd("");
    uv_chdir(cwd.c_str());
    [NSApp run];
    return shouldExit;
  }

  // We need to dispatch any code that touches
  // a window back into the main thread.
  void App::dispatch(std::function<void()> f) {
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        f();
      });
    });
  }

  void App::kill () {
    // Distinguish window closing with app exiting
    shouldExit = true;
    // if we were not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!fromSSC) {
      [NSApp terminate:nil];
    }
  }

  void App::restart () {
  }
}
