#include "process.h"
#include "util.h"
#include "cli.h"

constexpr auto version = STR_VALUE(VERSION);

void help () {
  std::cout
    << "opkit " << version
    << std::endl
    << std::endl
    << "usage:" << std::endl
    << "  opkit <project-dir> [-hpbc]"
    << std::endl
    << std::endl
    << "flags:" << std::endl
    << "  -h  help" << std::endl
    << "  -o  only run user build step" << std::endl
    << "  -xd turn off debug mode" << std::endl
    << "  -r  run after building" << std::endl
    << "  -b  bundle for app store" << std::endl
    << "  -c  code sign the bundle" << std::endl
    << "  -me (macOS) use entitlements" << std::endl
    << "  -mn (macOS) notarize the bundle" << std::endl
  ;

  exit(0);
}

auto start = std::chrono::system_clock::now();

void log (const std::string s) {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto delta = duration_cast<milliseconds>(now - start).count();
  std::cout << "• " << s << " \033[0;32m+" << delta << "ms\033[0m" << std::endl;
  start = std::chrono::system_clock::now();
}

static std::string getCxxFlags() {
  const char* flags = std::getenv("CXX_FLAGS");
  return flags ? " " + std::string(flags) : "";
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
  bool flagDebugMode = true;

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

    if (std::string(arg).find("-xd") != -1) {
      flagDebugMode = false;
    }
  }

  //
  // TODO(@heapwolf) split path values from the settings file
  // on the os separator to make them work cross-platform.
  //

  auto target = fs::path(argv[1]);

  auto _settings = readFile(fs::path { target / "settings.config" });
  auto settings = parseConfig(_settings);

  auto pathOutput = fs::path { settings["output"] };

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

  //
  // Darwin Package Prep
  // -------------------
  //
  if (platform.darwin) {
    log("preparing build for darwin");
    flags = "-DWEBVIEW_COCOA -std=c++2a -framework WebKit -framework AppKit";
    flags += getCxxFlags();

    files += prefixFile("src/main.cc");
    files += prefixFile("src/process_unix.cc");
    files += prefixFile("src/darwin.mm");

    fs::path pathBase = "Contents";
    packageName = fs::path(std::string(settings["title"] + ".app"));

    pathPackage = { target / pathOutput / packageName };
    pathBin = { pathPackage / pathBase / "MacOS" };
    pathResources = { pathPackage / pathBase / "Resources" };

    pathResourcesRelativeToUserBuild = {
      settings["output"] /
      packageName /
      pathBase /
      "Resources"
    };

    fs::create_directories(pathBin);
    fs::create_directories(pathResources);

    auto plistInfo = tmpl(gPListInfo, settings);

    writeFile(fs::path {
      pathPackage /
      pathBase /
      "Info.plist"
    }, plistInfo);
  }

  //
  // Linux Package Prep
  // ------------------
  //
  if (platform.linux) {
    log("preparing build for linux");
    flags = "-DWEBVIEW_GTK -std=c++2a `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0`";
    flags += getCxxFlags();

    files += prefixFile("src/main.cc");
    files += prefixFile("src/process_unix.cc");
    files += prefixFile("src/linux.cc");

    // this follows the .deb file naming convention
    packageName = fs::path((
      settings["executable"] + "_" +
      settings["version"] + "-" +
      settings["revision"] + "_" +
      settings["arch"]
    ));

    pathPackage = { target / pathOutput / packageName };

    fs::path pathBase = {
      pathPackage /
      "opt" /
      settings["name"]
    };

    pathBin = pathBase;
    pathResources = pathBase;

    pathResourcesRelativeToUserBuild = {
      pathOutput /
      packageName /
      "opt" /
      settings["name"]
    };

    fs::path pathControlFile = {
      pathPackage /
      "DEBIAN"
    };

    fs::path pathManifestFile = {
      pathPackage /
      "usr" /
      "share" /
      "applications"
    };

    fs::path pathIcons = {
      pathPackage /
      "usr" /
      "share" /
      "icons" /
      "hicolor" /
      "256x256" /
      "apps"
    };

    fs::create_directories(pathIcons);
    fs::create_directories(pathResources);
    fs::create_directories(pathManifestFile);
    fs::create_directories(pathControlFile);

    writeFile(fs::path {
      pathManifestFile /
      (settings["name"] + ".desktop")
    }, tmpl(gDestkopManifest, settings));

    writeFile(fs::path {
      pathControlFile /
      "control"
    }, tmpl(gDebianManifest, settings));

    auto pathToIconSrc = pathToString(fs::path {
      target /
      settings["linux_icon"]
    });

    auto pathToIconDest = pathToString(fs::path {
      pathIcons /
      (settings["executable"] + ".png")
    });

    if (!fs::exists(pathToIconDest)) {
      fs::copy(pathToIconSrc, pathToIconDest);
    }
  }

  //
  // Win32 Package Prep
  // ------------------
  //
  if (platform.win32) {
    log("preparing build for win32");
    flags = "-mwindows -L./dll/x64 -lwebview -lWebView2Loader";
    files += prefixFile("src/main.cc");
    files += prefixFile("process_win32.cc");
    files += prefixFile("src/win32.cc");

    // TODO create paths, copy files, archive, etc.
  }

  log("package prepared");

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

  // Serialize the settings to pass them to the compiler
  // by replacing new lines with a high bit.
  _settings = replace(_settings, "\n", "%%");

  compileCommand
    << " " << std::getenv("CXX")
    << " " << files
    << " " << flags
    << " " << settings["flags"]
    << " -o " << pathToString(binaryPath)
    << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
    << " " << define("SETTINGS", _settings);

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
        (settings["title"] + ".entitlements")
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
      (settings["executable"] + ".zip")
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

    while (!uuid.empty()) {
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
