#include "platform.h"
#include "build.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <regex>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
auto start = std::chrono::system_clock::now();

std::string pathToString(const fs::path &path) {
  auto s = path.u8string();
  return std::string(s.begin(), s.end());
}

std::string readFile (fs::path path) {
  std::ifstream stream(path.u8string());
  std::string content;
  auto buffer = std::istreambuf_iterator<char>(stream);
  auto end = std::istreambuf_iterator<char>();
  content.assign(buffer, end);
  stream.close();
  return content;
}

void writeFile (fs::path path, std::string s) {
  std::ofstream stream(pathToString(path));
  stream << s;
  stream.close();
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

std::string replace (const std::string s, std::map<std::string, std::string> pairs) {
  std::string output = s;

  for (auto item : pairs) {
    auto key = "[{]+(" + item.first + ")[}]+";
    auto value = item.second;
    output = std::regex_replace(output, std::regex(key), value);
  }

  return output;
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
  if (argc < 2) {
    std::cout << "usage: opkit <project-dir>" << std::endl;
    exit(0);
  }

  Platform platform;
  auto target = fs::path(argv[1]);

  auto settings = readConfig(fs::path { target / "settings.config" });
  auto menu = readFile(fs::path { target / "menu.config" });

  // TODO split path on os sep to make output path cross-platform.
  auto pathOutput = fs::path { target / fs::path(settings["output"]) };

  fs::remove_all(pathOutput);
  log("cleaned: " + pathToString(pathOutput));

  auto title = settings["title"];
  std::string flags;
  std::string files;

  fs::path pathBin;
  fs::path pathResources;
  fs::path pathResourcesRelativeToUserBuild;
  fs::path pathPackage;

  if (platform.darwin) {
    flags = "-luv -std=c++2a -framework WebKit -framework AppKit";
    files = "src/main.cc src/darwin.mm";

    fs::path pathBase = "Contents";
    fs::path packageName = fs::path(std::string(title + ".app"));

    pathPackage = { pathOutput / packageName };
    pathBin = { pathPackage / pathBase / fs::path { "MacOS" } };
    pathResources = { pathPackage / pathBase / fs::path { "Resources" } };
    
    pathResourcesRelativeToUserBuild = {
      fs::path(settings["output"]) /
      packageName /
      pathBase /
      fs::path { "Resources" }
    };

    fs::create_directories(pathBin);
    fs::create_directories(pathResources);

    auto plistInfo = replace(gPListInfo, settings);
    writeFile(fs::path { pathPackage / pathBase / fs::path("Info.plist") }, plistInfo);
    // Replace and write list files
  }

  if (platform.linux) {
    flags = "`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0'`";
    files = "src/main.cc src/linux.cc";

    pathResourcesRelativeToUserBuild = {
      fs::path(settings["output"])
      //
      //
      //
      // TODO where is this going?
      //
      //
      //
      //
    };
  }

  if (platform.win32) {
    flags = "-mwindows -L./dll/x64 -lwebview -lWebView2Loader";
    files = "src/main.cc src/win32.cc";

    // TODO build specifics
  }

  //
  // TODO copy the ipc module into the bundle
  // and maybe some other things too...
  //
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
    << pathToString(target)
    << " && "
    << settings["build"]
    << " "
    << pathToString(pathResourcesRelativeToUserBuild);

  std::cout << buildCommand.str() << std::endl;

  std::system(buildCommand.str().c_str());
  log("ran user build command");

  std::stringstream compileCommand;
  fs::path binaryPath = { pathBin / fs::path(title) };

  // Create flags for compile-time definitions.
  std::string flagPrefix = platform.win32 ? "/" : "-";
  auto define = [&](const std::string label, const std::string s) -> std::string {
    return std::string(flagPrefix + "D" + label + "=\"\\\"" + s + "\\\"\"");
  };

  // Serialize the menu to pass it to the compiler.
  std::replace(menu.begin(), menu.end(), '\n', '_');

  compileCommand
    << settings["compiler"] << " "
    << files << " "
    << flags << " "
    << settings["flags"] << " "
    << "-o " << pathToString(binaryPath) << " "
    << define("WIN_TITLE", settings["title"]) << " "
    << define("WIN_WIDTH", settings["width"]) << " "
    << define("WIN_HEIGHT", settings["height"]) << " "
    << define("CMD", settings["cmd"]) << " "
    << define("MENU", menu) << " "
    << define("ARG", settings["arg"]);

  // log(compileCommand.str());
  std::system(compileCommand.str().c_str());
  log("compiled native binary");
  return 0;
}
