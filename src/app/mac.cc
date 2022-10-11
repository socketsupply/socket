#include "../window/window.hh"
#include "app.hh"

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("co.socketsupply.queue.app", qos);

namespace SSC {
  App::App (int) : App() {
  }

  App::App () {
    this->core = new Core();
    this->bridge.core = this->core;
  }

  String App::getCwd () {
    NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
    return String([bundlePath UTF8String]);
  }

  int App::run () {
    auto cwd = getCwd();
    uv_chdir(cwd.c_str());
    [NSApp run];
    return shouldExit;
  }

  void App::dispatch (std::function<void()> callback) {
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
      });
    });
  }

  void App::kill () {
    // Distinguish window closing with app exiting
    shouldExit = true;
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!fromSSC) {
      [NSApp terminate:nil];
    }
  }

  void App::restart () {
  }
}
