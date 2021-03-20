#include "platform.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
auto start = std::chrono::system_clock::now();

std::string readFile (fs::path path) {
  std::ifstream stream(path.u8string());
  std::string content;
  auto buffer = std::istreambuf_iterator<char>(stream);
  auto end = std::istreambuf_iterator<char>();
  content.assign(buffer, end);
  return content;
}

std::map<std::string, std::string> readConfig(const fs::path p) {
  if (!fs::exists(p)) {
    std::cout << p << " does not exist" << std::endl;
    exit(1);
  }

  auto source = readFile(p);
  auto entries = split(source, '\n');
  std::map<std::string, std::string> settings;

  for (auto entry : entries) {
    auto pair = split(entry, ':');

    if (pair.size() == 2) {
      settings[trim(pair[0])] = trim(pair[1]);
    }
  }

  return settings;
}

void log (const std::string s) {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto delta = duration_cast<milliseconds>(now - start).count();
  std::cout << "• " << s << " " << delta << "ms" << std::endl;
}

int exec (std::string cmd, std::string s) {
  std::string c = std::string(cmd + " " + s);
  return system(c.c_str());
}

int main (const int argc, const char* argv[]) {
  if (argc < 1) {
    std::cout << "Config file not specified" << std::endl;
  }

  Platform platform;
  auto target = fs::path(argv[1]);

  auto settings = readConfig(fs::path { target / "settings.config" });
  auto menu = readFile(fs::path { target / "menu.config" });

  // TODO split path on os sep to make output path cross-platform.
  auto pathOutput = fs::path { target / fs::path(settings["output"]) };

  fs::remove_all(pathOutput);
  log("cleaned: " + pathOutput.u8string());

  auto title = settings["title"];

  fs::path pathBin;
  fs::path pathResources;
  fs::path pathPackage;

  if (platform.darwin) {
    fs::path pathBase = "Contents";
    fs::path packageName = fs::path(std::string(title + ".app"));

    pathPackage = { pathOutput / packageName };
    pathBin = { pathPackage / pathBase / fs::path { "MacOS" } };
    pathResources = { pathPackage / pathBase / fs::path { "Resources" } };

    fs::create_directories(pathBin);
    fs::create_directories(pathResources);
  }

  if (platform.linux) {
    // TODO build step
  }

  if (platform.win32) {
    // TODO build step
  }

  // copy the ipc stuff over
  // fs::copy(

  log("created app structure");

  //
  // cd into the target and run the user's build command,
  // pass it the platform specific directory where they
  // should send their build artifacts.
  //
  std::stringstream buildCommand;

  buildCommand
    << "cd "
    << target.u8string()
    << " && "
    << settings["build"]
    << " "
    << pathResources;

  std::system(buildCommand.str().c_str());
  log("ran user build command");

  std::stringstream compileCommand;
  std::string flagPrefix = platform.win32 ? "/" : "-";
  fs::path binaryPath = { pathBin / fs::path(title) };

  auto def = [&](const std::string label, const std::string s) -> std::string {
    return std::string(flagPrefix + "D" + label + "=\"" + s + "\"");
  };

  std::replace(menu.begin(), menu.end(), '\n', '_');

  compileCommand
    << settings["compiler"] << " "
    << settings["files_" + platform.os] << " "
    << settings["flags_" + platform.os] << " "
    << settings["optimizations"] << " "
    << "-o " << binaryPath.u8string() << " "
    << def("O_WIN_TITLE", settings["title"]) << " "
    << def("O_WIN_WIDTH", settings["width"]) << " "
    << def("O_WIN_HEIGHT", settings["height"]) << " "
    << def("O_CMD", settings["cmd"]) << " "
    << def("O_MENU", menu) << " "
    << def("O_ARG", settings["arg"]);

  // log(compileCommand.str());
  std::system(compileCommand.str().c_str());
  log("compiled native binary");
  return 0;
}
