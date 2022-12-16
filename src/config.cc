#include <fstream>
#include <iostream>
#include <filesystem>
#include "config.hh"

int main (const int argc, const char* argv[]) {
  auto base = std::filesystem::path(argv[1]);
  std::filesystem::path dest = { base / ".ssc.dat"};

  std::ofstream ws(dest, std::ios::out | std::ios::binary);
  if (!ws) return 1;

  using namespace SSC;

  Config config;
  #include "ssc.conf" // NOLINT

  ws.write((char*) &config, sizeof(Config));
  ws.close();

  if (!ws.good()) return 1;

  return 0;
}
