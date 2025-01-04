#include "../app.hh"
#include "../cli.hh"

using ssc::app::App;

int main (int argc, char *argv[]) {
  struct rlimit limit;
  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = 2048;
  setrlimit(RLIMIT_NOFILE, &limit);
  @autoreleasepool {
    App app(App::Options {});
    return app.run(argc, argv);
  }
}
