#include "../app/app.hh"
#include "../cli/cli.hh"

int main (int argc, char *argv[]) {
  SSC::App app(0);
  struct rlimit limit;
  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = 2048;
  setrlimit(RLIMIT_NOFILE, &limit);
  return app.run(argc, argv);
}
