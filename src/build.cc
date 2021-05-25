#include "platform.h"
#include "build.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <span>
#include <regex>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

constexpr auto version = STR_VALUE(VERSION);
namespace fs = std::filesystem;
auto start = std::chrono::system_clock::now();
Platform platform;

std::string pathToString(const fs::path &path) {
  auto s = path.u8string();
  return std::string(s.begin(), s.end());
}

std::string readFile (fs::path path) {
  std::ifstream stream(path.c_str());
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

std::string prefixFile (std::string s) {
  if (platform.darwin || platform.linux) {
    return std::string("/usr/local/lib/opkit/" + s + " ");
  }

  return std::string("C:\\Program Files\\operator\\build\\" + s + " ");
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
  std::cout << "• " << s << " \033[0;32m+" << delta << "ms\033[0m" << std::endl;
  start = std::chrono::system_clock::now();
}

int exec (std::string cmd, std::string s) {
  std::string c = std::string(cmd + " " + s);
  return system(c.c_str());
}

void help () {
  std::cout
    << "Opkit " << version
    << std::endl
    << std::endl
    << "usage:" << std::endl
    << "  opkit <project-dir> [-hpbc]"
    << std::endl
    << std::endl
    << "flags:" << std::endl
    << "  -h  this help message" << std::endl
    << "  -o  only run user defined build step" << std::endl
    << "  -r  run the binary after building it" << std::endl
    << "  -b  bundle for app store" << std::endl
    << "  -c  code sign the bundle" << std::endl
    << "  -ce code sign the bundle with entitlements" << std::endl
  ;

  exit(0);
}

int main (const int argc, const char* argv[]) {
  if (argc < 2) {
    help();
  }

  if (std::getenv("CXX") == nullptr) {
    std::cout
      << "warning: $CXX environment variable not set, assuming '/usr/bin/g++'."
      << std::endl;
    setenv("CXX", "/usr/bin/g++", 0);
    exit(0);
  }

  bool flagRunUserBuild = false;
  bool appStore = false;
  bool codeSign = false;
  bool shouldRun = false;
  bool withEntitlements = false;

  for (auto const arg : std::span(argv, argc)) {
    if (std::string(arg).find("-h") != -1) {
      help();
    }

    if (std::string(arg).find("-o") != -1) {
      flagRunUserBuild = true;
    }

    if (std::string(arg).find("-s") != -1) {
      appStore = true;
    }

    if (std::string(arg).find("-c") != -1) {
      codeSign = true;
    }

    if (std::string(arg).find("-ce") != -1) {
      codeSign = true;
      withEntitlements = true;
    }
  }

  auto target = fs::path(argv[1]);

  auto settings = readConfig(fs::path { target / "settings.config" });
  auto menu = readFile(fs::path { target / "menu.config" });

  // TODO split output path variable on os sep to make output path cross-platform.
  auto pathOutput = fs::path { fs::path(settings["output"]) };

  if (flagRunUserBuild == false) {
    fs::remove_all(pathOutput);
    log("cleaned: " + pathToString(pathOutput));
  }

  auto executable = fs::path(platform.darwin
    ? settings["title"]
    : settings["executable"]);

  std::string flags;
  std::string files;

  fs::path pathBin;
  fs::path pathResources;
  fs::path pathResourcesRelativeToUserBuild;
  fs::path pathPackage;
  fs::path packageName;

  if (platform.darwin) {
    log("preparing build for darwin");
    flags = "-DWEBVIEW_COCOA -std=c++2a -framework WebKit -framework AppKit";

    files += prefixFile("src/main.cc");
    files += prefixFile("src/process_unix.cc");
    files += prefixFile("src/darwin.mm");

    fs::path pathBase = "Contents";
    packageName = fs::path(std::string(settings["title"] + ".app"));

    pathPackage = { target / pathOutput / packageName };
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

    writeFile(fs::path {
      pathPackage /
      pathBase /
      fs::path("Info.plist")
    }, plistInfo);
  }

  if (platform.linux) {
    log("preparing build for linux");
    flags = "-DWEBVIEW_GTK -std=c++2a `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0`";
    files += prefixFile("src/main.cc");
    files += prefixFile("src/process_unix.cc");
    files += prefixFile("src/linux.cc");

    // this follows the .deb file naming convention
    packageName = fs::path(std::string(
      settings["executable"] + "_" +
      settings["version"] + "-" +
      settings["revision"] + "_" +
      settings["arch"]
    ));

    pathPackage = { target / pathOutput / packageName };

    fs::path pathBase = {
      pathPackage /
      fs::path { "opt" } /
      fs::path { settings["name"] }
    };

    pathBin = pathBase;
    pathResources = pathBase;

    pathResourcesRelativeToUserBuild = {
      pathOutput /
      packageName /
      fs::path { "opt" } /
      fs::path { settings["name"] }
    };

    fs::path pathControlFile = {
      pathPackage /
      fs::path { "DEBIAN" }
    };

    fs::path pathManifestFile = {
      pathPackage /
      fs::path { "usr" } /
      fs::path { "share" } /
      fs::path { "applications" }
    };

    fs::path pathIcons = {
      pathPackage /
      fs::path { "usr" } /
      fs::path { "share" } /
      fs::path { "icons" } /
      fs::path { "hicolor" } /
      fs::path { "256x256" } /
      fs::path { "apps" }
    };

    fs::create_directories(pathIcons);
    fs::create_directories(pathResources);
    fs::create_directories(pathManifestFile);
    fs::create_directories(pathControlFile);

    writeFile(fs::path {
      pathManifestFile /
      fs::path(std::string(settings["name"] + ".desktop"))
    }, replace(gDestkopManifest, settings));

    writeFile(fs::path {
      pathControlFile /
      fs::path { "control" }
    }, replace(gDebianManifest, settings));

    // fs::copy("", ""); // icon to `pathIcons/<executable>.png`
  }

  if (platform.win32) {
    log("preparing build for win32");
    flags = "-mwindows -L./dll/x64 -lwebview -lWebView2Loader";
    files += prefixFile("src/main.cc");
    files += prefixFile("process_win32.cc");
    files += prefixFile("src/win32.cc");

    // TODO create paths, copy files, archive, etc.
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

  // log(buildCommand.str());
  std::system(buildCommand.str().c_str());
  log("ran user build command");

  std::stringstream compileCommand;
  fs::path binaryPath = { pathBin / executable };

  // Create flags for compile-time definitions.
  std::string flagPrefix = platform.win32 ? "/" : "-";
  auto define = [&](const std::string label, const std::string s) -> std::string {
    return std::string(flagPrefix + "D" + label + "=\"\\\"" + s + "\\\"\"");
  };

  // Serialize the menu to pass it to the compiler.
  std::replace(menu.begin(), menu.end(), '\n', '_');

  compileCommand
    << std::getenv("CXX") << " "
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
  if (flagRunUserBuild == false) {
    std::system(compileCommand.str().c_str());
    log("compiled native binary");
  }

  //
  // Archive step
  //
  if (platform.linux) {
    std::stringstream archiveCommand;

    archiveCommand
      << "dpkg-deb --build --root-owner-group "
      << pathToString(pathPackage)
      << " "
      << pathToString(fs::path { target / pathOutput });

    std::system(archiveCommand.str().c_str());
  }

  if (platform.darwin) {
    // create DMG package if MAS is desired
  }

  //
  // Code signing step
  //
  if (codeSign && platform.darwin) {
    //
    // References
    // ---
    // https://www.digicert.com/kb/code-signing/mac-os-codesign-tool.htm
    // https://developer.apple.com/forums/thread/128166
    // https://wiki.lazarus.freepascal.org/Code_Signing_for_macOS
    //
    std::stringstream signCommand;

    std::string entitlements = "";

    if (withEntitlements) {
      entitlements = std::string(
        " --entitlements " + pathToString(fs::path {
        pathResourcesRelativeToUserBuild /
        fs::path(std::string(settings["title"] + ".entitlements"))
      }));
    }

    signCommand
      << "codesign"
      << " --deep"
      << " --force"
      << " --options runtime"
      << " --timestamp"
      << entitlements
      << " --sign 'Developer ID Application: " + settings["mac_sign"] + "'"
      << " "
      << pathToString(pathPackage);

    std::system(signCommand.str().c_str());
    log("finished code signing");
  }

  if (platform.win32) {
    //
    // References
    // ---
    // https://www.digicert.com/kb/code-signing/signcode-signtool-command-line.htm
    //
  }

  return 0;
}
