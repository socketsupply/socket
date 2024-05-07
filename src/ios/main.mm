#include "../app/app.hh"
#include "../cli/cli.hh"

int main (int argc, char *argv[]) {
  struct rlimit limit;
  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = 2048;
  setrlimit(RLIMIT_NOFILE, &limit);
  @autoreleasepool {
    SSC::App app(0);
    return app.run(argc, argv);
  }
}
