#include "platform.h"
#include "process.h"
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

std::string exec (std::string command) {
  FILE *pipe;
  char buf[128];

#ifdef _WIN32
  //
  // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen?view=msvc-160
  // _popen works fine in a console application... ok fine that's all we need it for... thanks.
  //
  pipe = _popen((const char*) command.c_str(), "rt");
#else
  pipe = popen((const char*) command.c_str(), "r");
#endif

  if (pipe == NULL) {
    std::cout << "error: unable to opent he command" << std::endl;
    exit(1);
  }

  std::stringstream ss;

  while (fgets(buf, 128, pipe)) {
    ss << buf;
  }

#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif

  return ss.str();
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
    << "  -h  help" << std::endl
    << "  -o  only run user build step" << std::endl
    << "  -r  run after building" << std::endl
    << "  -b  bundle for app store" << std::endl
    << "  -c  code sign the bundle" << std::endl
    << "  -me (macOS) use entitlements" << std::endl
    << "  -mn (macOS) notarize the bundle" << std::endl
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
  bool flagAppStore = false;
  bool flagCodeSign = false;
  bool flagShouldRun = false;
  bool flagEntitlements = false;
  bool flagNotarization = false;

  for (auto const arg : std::span(argv, argc)) {
    if (std::string(arg).find("-h") != -1) {
      help();
    }

    if (std::string(arg).find("-o") != -1) {
      flagRunUserBuild = true;
    }

    if (std::string(arg).find("-s") != -1) {
      flagAppStore = true;
    }

    if (std::string(arg).find("-c") != -1) {
      flagCodeSign = true;
    }

    if (std::string(arg).find("-me") != -1) {
      flagEntitlements = true;
    }

    if (std::string(arg).find("-mn") != -1) {
      flagNotarization = true;
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
  fs::path pathToArchive;
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
    << " cd "
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
    exec(compileCommand.str());
    log("compiled native binary");
  }

  //
  // Linux Packaging
  // ---------------
  //
  if (platform.linux) {
    std::stringstream archiveCommand;

    archiveCommand
      << "dpkg-deb --build --root-owner-group "
      << pathToString(pathPackage)
      << " "
      << pathToString(fs::path { target / pathOutput });

    auto r = std::system(archiveCommand.str().c_str());

    if (r != 0) {
      log("error: failed to create deb package");
      exit(1);
    }
  }

  //
  // MacOS Code Signing
  // ------------------
  //
  if (flagCodeSign && platform.darwin) {
    //
    // https://www.digicert.com/kb/code-signing/mac-os-codesign-tool.htm
    // https://developer.apple.com/forums/thread/128166
    // https://wiki.lazarus.freepascal.org/Code_Signing_for_macOS
    //
    std::stringstream signCommand;
    std::string entitlements = "";

    if (flagEntitlements) {
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

    exec(signCommand.str());
    log("finished code signing");
  }

  //
  // MacOS Packaging
  // ---------------
  //
  if (platform.darwin) {
    std::stringstream zipCommand;

    pathToArchive = fs::path {
      target /
      pathOutput /
      fs::path(std::string(settings["executable"] + ".zip"))
    };

    zipCommand
      << "ditto"
      << " -c"
      << " -k"
      << " --keepParent"
      << " --sequesterRsrc"
      << " "
      << pathToString(pathPackage)
      << " "
      << pathToString(pathToArchive);

    auto r = std::system(zipCommand.str().c_str());

    if (r != 0) {
      log("error: failed to create zip for notarization");
      exit(1);
    }

    log("craeted zip artifact");
  }

  //
  // MacOS Notorization
  // ------------------
  //
  if (flagNotarization && platform.darwin) {
    std::stringstream notarizeCommand;

    notarizeCommand
      << "xcrun"
      << " altool"
      << " --notarize-app"
      << " -f"
      << pathToString(pathToArchive)
      << " --primary-bundle-id"
      << " "
      << settings["mac_bundle_identifier"];

    auto stdout = exec(notarizeCommand.str().c_str());

    std::regex re(R"(\nRequestUUID = (.+?)\n)");
    std::smatch match;
    std::string uuid;

    if (std::regex_search(stdout, match, re)) {
      uuid = match.str(1);
    }

    int requests = 0;

    std::cout << "polling for notarization";

    while (uuid) {
      if (++requests > 1024) {
        log("apple did not respond to the request for notarization");
        exit(1);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1024 * 8));
      std::stringstream notarizeStatusCommand;

      notarizeStatusCommand
        << "xcrun"
        << " altool"
        << " --notarization-info"
        << " " << uuid;

      auto stdout = exec(notarizeStatusCommand.str().c_str());

      std::regex re(R"(\n *Status: (.+?)\n)");
      std::smatch match;
      std::string status;

      if (std::regex_search(stdout, match, re)) {
        status = match.str(1);
      }

      if (status.find("in progress") != -1) {
        std::cout << ".";
        continue;
      }

      if (status.find("invalid") != -1) {
        log("apple rejected the request for notarization");
        exit(1);
      }

      if (status.find("success") != -1) {
        log("successfully notarized");
      }

      if (status.find("success") == -1) {
        log("apple was unable to notarize");
      }
    }

    log("finished notarization");
  }

  //
  // Win32 Packaging
  // ---------------
  //
  if (platform.win32) {
    //
    // https://www.digicert.com/kb/code-signing/signcode-signtool-command-line.htm
    // https://github.com/wixtoolset/
    //
  }

  return 0;
}
