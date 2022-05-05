#include "common.hh"
#include "cli.hh"
#include "ios.hh"
#include "process.hh"
#include <filesystem>

#ifdef _WIN32
#include <shlwapi.h>
#include <strsafe.h>
#include <comdef.h>
#include <AppxPackaging.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Urlmon.lib")
#define WEXITSTATUS(w) ((int) ((w) & 0x40000000))
#endif

#ifndef CMD_RUNNER
#define CMD_RUNNER
#endif

using namespace Operator;
using namespace std::chrono;

constexpr auto version_hash = STR_VALUE(VERSION_HASH);
constexpr auto version = STR_VALUE(VERSION);
constexpr auto full_version = STR_VALUE(VERSION) " (" STR_VALUE(VERSION_HASH) ")";
auto start = std::chrono::system_clock::now();
bool porcelain = false;

void log (const std::string s) {
  if (porcelain) return;

  #ifdef _WIN32 // unicode console support
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
  #endif

  auto now = system_clock::now();
  auto delta = duration_cast<milliseconds>(now - start).count();
  std::cout << "• " << s << " \033[0;32m+" << delta << "ms\033[0m" << std::endl;
  start = std::chrono::system_clock::now();
}

void init (Map attrs) {
  auto cwd = fs::current_path();
  fs::create_directories(cwd / "src");
  Operator::writeFile(cwd / "src" / "index.html", "<html>Hello, World</html>");
  Operator::writeFile(cwd / "operator.config", tmpl(gDefaultConfig, attrs));
}

static std::string getCxxFlags() {
  auto flags = getEnv("CXX_FLAGS");
  return flags.size() > 0 ? " " + flags : "";
}

int main (const int argc, const char* argv[]) {
  Map attrs;
  attrs["version"] = version;

  if (argc < 2) {
    std::cout << tmpl(gHelpText, attrs) << std::endl;
    exit(0);
  }

  bool flagRunUserBuild = false;
  bool flagAppStore = false;
  bool flagCodeSign = false;
  bool flagShouldRun = false;
  bool flagEntitlements = false;
  bool flagShouldNotarize = false;
  bool flagDebugMode = true;
  bool flagTestMode = false;
  bool flagShouldPackage = false;
  bool flagBuildForIOS = false;
  bool flagBuildForAndroid = false;
  bool flagBuildForSimulator = false;
  bool flagPrintBuildPath = false;

  std::string devPort("0");

  auto is = [](const std::string& s, const auto& p) -> bool {
    return s.compare(p) == 0;
  };

  for (auto const arg : std::span(argv, argc)) {
    if (is(arg, "-c")) {
      flagCodeSign = true;
    }

    if (is(arg, "-h")) {
      std::cout << tmpl(gHelpText, attrs) << std::endl;
      exit(0);
    }

    if (is(arg, "-i")) {
      init(attrs);
      exit(0);
    }

    if (is(arg, "-v")) {
      std::cout << full_version << std::endl;
      exit(0);
    }

    if (is(arg, "-me")) {
      flagEntitlements = true;
    }

    if (is(arg, "-mn")) {
      flagShouldNotarize = true;
    }

    if (is(arg, "-mid")) {
      std::string command = "system_profiler SPUSBDataType -json";
      auto r = exec(command);

      if (r.exitCode == 0) {
        std::regex re(R"REGEX("(?:iPhone(?:(?:.|\n)*?)"serial_num"\s*:\s*"([^"]*))")REGEX");
        std::smatch match;
        std::string uuid;

        if (!std::regex_search(r.output, match, re)) {
          log("failed to extract device uuid using \"" + command + "\"");
          log("Is the device plugged in?");
          exit(1);
        }

        std::cout << std::string(match[1]).insert(8, 1, '-') << std::endl;
        exit(0);
      } else {
        log("Could not get device id. Is the device plugged in?");
        exit(1);
      }
    }

    if (is(arg, "-o")) {
      flagRunUserBuild = true;
    }

    if (is(arg, "-p")) {
      flagShouldPackage = true;
    }

    if (is(arg, "-r")) {
      flagShouldRun = true;
    }

    if (is(arg, "-s")) {
      flagAppStore = true;
    }

    if (is(arg, "-xd")) {
      flagDebugMode = false;
    }

    if (is(arg, "-ios")) {
      flagBuildForIOS = true;
    }

    if (is(arg, "-android")) {
      flagBuildForAndroid = true;
    }

    if (is(arg, "-simulator")) {
      flagBuildForSimulator = true;
    }

    if (is(arg, "--test")) {
      flagTestMode = true;
    }

    if (is(arg, "--target")) {
      porcelain = true;
      flagPrintBuildPath = true;
    }

    if (std::string(arg).find("--port=") == 0) {
      devPort = std::string(arg).substr(7);
    }
  }

  if (getEnv("CXX").size() == 0) {
    log("warning! $CXX env var not set, assuming defaults");

    if (platform.win) {
      setEnv("CXX=clang++");
    } else {
      setEnv("CXX=/usr/bin/g++");
    }
  }

  //
  // TODO(@heapwolf) split path values from the settings file
  // on the os separator to make them work cross-platform.
  //

  auto target = fs::path(argv[1]);

  if (argv[1][0] == '-') {
    log("a directory was expected as the first argument");
    exit(0);
  }

  if (argv[1][0] == '.') {
    target = fs::absolute(target);
  }

  auto _settings = WStringToString(readFile(target / "operator.config"));
  auto settings = parseConfig(_settings);

  bool noCommand = (
    settings.count("win_cmd") == 0 &&
    settings.count("mac_cmd") == 0 &&
    settings.count("linux_cmd") == 0
  );

  if (noCommand) {
    log("No entry in config file for 'win_cmd', 'mac_cmd', or 'linux_cmd'");
  }

  if (noCommand && devPort.size() == 0) {
    log("Try specifying a port with '--port=8080'.");
    exit(1);
  }

  if (settings.count("revision") == 0) {
    settings["revision"] = "1";
  }

  if (settings.count("arch") == 0 || settings["arch"] == "auto") {
    settings["arch"] = platform.arch;
  }

  std::vector<std::string> required = {
    "name",
    "title",
    "executable",
    "output",
    "version",
    "arch"
  };

  for (const auto &str : required) {
    if (settings.count(str) == 0) {
      log("'" + str + "' key/value is required");
      exit(1);
    }
  }

  std::string suffix = "";

  if (flagDebugMode) {
    suffix += "-dev";
  }

  if (flagTestMode) {
    suffix += "-test";
  }

  settings["name"] += suffix;
  settings["title"] += suffix;
  settings["executable"] += suffix;

  auto pathOutput = settings["output"];

  if (flagRunUserBuild == false && fs::exists(pathOutput)) {
    auto p = fs::current_path() / pathOutput;

    try {
      fs::remove_all(p);
      log(std::string("cleaned: " + p.string()));
    } catch (fs::filesystem_error const& ex) {
      log("could not clean path (binary could be busy)");
      log(std::string("ex: ") + ex.what());
      log(std::string("ex code: ") + std::to_string(ex.code().value()));
      exit(1);
    }
  }

  auto executable = fs::path(
    settings["executable"] + (platform.win ? ".exe" : ""));

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
  // ---
  //
  if (platform.mac && !flagBuildForIOS) {
    packageName = fs::path(std::string(settings["name"] + ".app"));
    pathPackage = { target / pathOutput / packageName };

    log("preparing build for mac");

    flags = "-std=c++2a -framework UniformTypeIdentifiers -framework WebKit -framework Cocoa -ObjC++";
    flags += getCxxFlags();

    files += prefixFile("src/main.cc");
    files += prefixFile("src/process_unix.cc");

    fs::path pathBase = "Contents";

    pathBin = { pathPackage / pathBase / "MacOS" };
    pathResources = { pathPackage / pathBase / "Resources" };

    pathResourcesRelativeToUserBuild = {
      pathOutput /
      packageName /
      pathBase /
      "Resources"
    };

    fs::create_directories(pathBin);
    fs::create_directories(pathResources);

    auto plistInfo = tmpl(gPListInfo, settings);

    writeFile(pathPackage / pathBase / "Info.plist", plistInfo);
  }

  if (platform.mac && flagBuildForIOS) {
    fs::remove_all(target / "dist");

    auto projectName = (settings["name"] + ".xcodeproj");
    auto schemeName = (settings["name"] + ".xcscheme");
    auto pathToProject = target / pathOutput / projectName;
    auto pathToScheme = pathToProject / "xcshareddata" / "xcschemes";
    auto pathToProfile = target / settings["ios_provisioning_profile"];

    fs::create_directories(pathToProject);
    fs::create_directories(pathToScheme);

    std::string command = (
      "security cms -D -i " + pathToProfile.string()
    );

    auto r = exec(command);
    std::regex re(R"(<key>UUID<\/key>\n\s*<string>(.*)<\/string>)");
    std::smatch match;
    std::string uuid;

    if (!std::regex_search(r.output, match, re)) {
      log("failed to extract uuid from provisioning profile using \"" + command + "\"");
      exit(1);
    }

    uuid = match.str(1);

    auto pathToInstalledProfile = fs::path(getEnv("HOME")) /
      "Library" /
      "MobileDevice" /
      "Provisioning Profiles" /
      (uuid + ".mobileprovision");

    if (!fs::exists(pathToInstalledProfile)) {
      fs::copy(pathToProfile, pathToInstalledProfile);
    }

    settings["ios_provisioning_profile"] = uuid;

    fs::copy(
      fs::path(prefixFile()) / "lib",
      target / pathOutput / "lib",
      fs::copy_options::overwrite_existing | fs::copy_options::recursive
    );

    writeFile(target / pathOutput / "exportOptions.plist", tmpl(gXCodeExportOptions, settings));
    writeFile(target / pathOutput / "Info.plist", tmpl(gXCodePlist, settings));
    writeFile(pathToProject / "project.pbxproj", tmpl(gXCodeProject, settings));
    writeFile(pathToScheme / schemeName, tmpl(gXCodeScheme, settings));

    pathResources = fs::path { target / pathOutput / "ui" };
    fs::create_directories(pathResources);

    pathResourcesRelativeToUserBuild = fs::path(pathOutput) / "ui";
  }

  //
  // Linux Package Prep
  // ---
  //
  if (platform.linux) {
    log("preparing build for linux");
    flags = " -std=c++2a `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0`";
    flags += " " + getCxxFlags();

    files += prefixFile("src/main.cc");
    files += prefixFile("src/process_unix.cc");

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

    auto linuxExecPath =
      fs::path("/opt") /
      settings["name"] /
      settings["executable"];

    settings["linux_executable_path"] = linuxExecPath.string();
    settings["linux_icon_path"] = (
      fs::path("/usr") /
      "share" /
      "icons" /
      "hicolor" /
      "256x256" /
      "apps" /
      (settings["executable"] + ".png")
    ).string();

    writeFile(pathManifestFile / (settings["name"] + ".desktop"), tmpl(gDestkopManifest, settings));

    if (settings.count("deb_name") == 0) {
      settings["deb_name"] = settings["name"];
    }

    writeFile(pathControlFile / "control", tmpl(gDebianManifest, settings));

    auto pathToIconSrc = (target / settings["linux_icon"]).string();
    auto pathToIconDest = (pathIcons / (settings["executable"] + ".png")).string();

    if (!fs::exists(pathToIconDest)) {
      fs::copy(pathToIconSrc, pathToIconDest);
    }
  }

  //
  // Windows Package Prep
  // ---
  //
  if (platform.win) {
    log("preparing build for win");
    auto prefix = prefixFile();

    flags = " -std=c++20"
      " -I" + prefix +
      " -I" + prefix + "\\src\\win64"
      " -L" + prefix + "\\src\\win64"
    ;

    files += prefixFile("src\\main.cc");
    files += prefixFile("src\\process_win.cc");

    packageName = fs::path((
      settings["executable"] + "-" +
      settings["version"]
    ));

    pathPackage = { target / pathOutput / packageName };
    pathBin = pathPackage;

    pathResourcesRelativeToUserBuild = pathPackage;

    fs::create_directories(pathPackage);

    auto p = fs::path {
      pathResourcesRelativeToUserBuild /
      "AppxManifest.xml"
    };

    if (settings["version_short"].size() > 0) {
      auto versionShort = settings["version_short"];
      auto winversion = split(versionShort, '-')[0];

      settings["win_version"] = winversion + ".0";
    } else {
      settings["win_version"] = "0.0.0.0";
    }

    settings["exe"] = settings["executable"] + ".exe";

    writeFile(p, tmpl(gWindowsAppManifest, settings));

    // TODO Copy the files into place
  }

  if (flagPrintBuildPath) {
    std::cout << pathResourcesRelativeToUserBuild.string();
    exit(0);
  }

  log("package prepared");

  std::stringstream argvForward;

  int c = 0;
  for (auto const arg : std::span(argv, argc)) {
    c++;
    auto argStr = std::string(arg);

    if (c > 2) {
      if ("-r" == argStr || "-o" == argStr) continue;
      if (argStr.find("-") != 0) continue;
      argvForward << " " << argStr;
    }
  }

  //
  // cd into the target and run the user's build command,
  // pass it the platform specific directory where they
  // should send their build artifacts.
  //
  std::stringstream buildCommand;
  auto oldCwd = fs::current_path();
  fs::current_path(oldCwd / target);

  {
    char prefix[4096] = {0};
    memcpy(
      prefix,
      pathResourcesRelativeToUserBuild.string().c_str(),
      pathResourcesRelativeToUserBuild.string().size()
    );

    // @TODO(jwerle): use `setEnv()` if #148 is closed
    #if _WIN32
      setEnv("PREFIX=prefix");
    #else
      setenv("PREFIX", prefix, 1);
    #endif
  }

  buildCommand
    << settings["build"]
    << " "
    << pathResourcesRelativeToUserBuild.string()
    << " --debug=" << flagDebugMode
    << argvForward.str();

  log(buildCommand.str());
  auto r = exec(buildCommand.str().c_str());

  if (r.exitCode != 0) {
    log("Unable to run user build command");
    exit(WEXITSTATUS(r.exitCode));
  }

  log(r.output);
  log("ran user build command");

  fs::current_path(oldCwd);

  if (flagBuildForIOS) {
    log("building for iOS");

    auto oldCwd = fs::current_path();
    auto pathToDist = oldCwd / target / "dist";

    fs::create_directories(pathToDist);
    fs::current_path(pathToDist);

    //
    // Copy and or create the source files we need for the build.
    //
    fs::copy(trim(prefixFile("src/ios.mm")), pathToDist);
    fs::copy(trim(prefixFile("src/ios.hh")), pathToDist);
    fs::copy(trim(prefixFile("src/common.hh")), pathToDist);
    fs::copy(trim(prefixFile("src/preload.hh")), pathToDist);

    auto pathBase = pathToDist / "Base.lproj";
    fs::create_directories(pathBase);

    writeFile(pathBase / "LaunchScreen.storyboard", gStoryboardLaunchScreen);
    // TODO allow the user to copy their own if they have one
    writeFile(pathToDist / "op.entitlements", gXcodeEntitlements);

    //
    // For iOS we're going to bail early and let XCode infrastructure handle
    // building, signing, bundling, archiving, noterizing, and uploading.
    //
    std::stringstream archiveCommand;
    std::string destination = "generic/platform=iOS";

    if (flagBuildForSimulator) {
      destination = settings["ios_device_simulator"];
    }

    if (settings["ios_codesign_identity"].size() == 0) {
      settings["ios_codesign_identity"] = "iPhone Distribution";
    }

    auto sup = std::string("archive");
    auto configuration = std::string(flagDebugMode ? "Debug" : "Release");

    if (!flagShouldPackage) {
      sup = "CONFIGURATION_BUILD_DIR=" + pathToDist.string();
    }

    archiveCommand
      << "xcodebuild"
      << " build " << sup
      << " -configuration " << configuration
      << " -scheme " << settings["name"]
      << " -destination '" << destination << "'";

    if (flagShouldPackage) {
      archiveCommand << " -archivePath build/" << settings["name"];
    }

    if (!flagCodeSign) {
      archiveCommand
        << " CODE_SIGN_IDENTITY=\"\""
        << " CODE_SIGNING_REQUIRED=\"NO\""
        << " CODE_SIGN_ENTITLEMENTS=\"\""
        << " CODE_SIGNING_ALLOWED=\"NO\"";
    }

    log(archiveCommand.str().c_str());
    auto rArchive = exec(archiveCommand.str().c_str());

    if (rArchive.exitCode != 0) {
      log("error: failed to archive project");
      fs::current_path(oldCwd);
      exit(1);
    }

    log("created archive");

    if (flagShouldPackage) {
      std::stringstream exportCommand;

      exportCommand
        << "xcodebuild"
        << " -exportArchive "
        << " -archivePath build/" << settings["name"] << ".xcarchive"
        << " -exportPath build/" << settings["name"] << ".ipa"
        << " -exportOptionsPlist " << (pathToDist / "exportOptions.plist").string();

      log(exportCommand.str());
      auto rExport = exec(exportCommand.str().c_str());

      if (rExport.exitCode != 0) {
        log("error: failed to export project");
        fs::current_path(oldCwd);
        exit(1);
      }

      log("exported archive");
    }

    if (flagShouldRun) {
      auto pathToXCode = fs::path("/Applications") / "Xcode.app" / "Contents";
      auto pathToSimulator = pathToXCode / "Developer" / "Applications" / "Simulator.app";

      std::stringstream simulatorCommand;
      simulatorCommand << "open " << pathToSimulator.string();
      exec(simulatorCommand.str().c_str());
    }

    log("completed");
    fs::current_path(oldCwd);
    return 0;
  }

  std::stringstream compileCommand;
  auto binaryPath = pathBin / executable;

  auto extraFlags = flagDebugMode
    ? settings.count("debug_flags") ? settings["debug_flags"] : ""
    : settings.count("flags") ? settings["flags"] : "";

  compileCommand
    << getEnv("CXX")
    << " " << files
    << " " << flags
    << " " << extraFlags
    << " -o " << binaryPath.string()
    << " -D_IOS=" << (flagBuildForIOS ? 1 : 0)
    << " -D_ANDROID=" << (flagBuildForAndroid ? 1 : 0)
    << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
    << " -DPORT=" << devPort
    << " -DSETTINGS=\"" << encodeURIComponent(_settings) << "\""
  ;

  // log(compileCommand.str());

  auto binExists = fs::exists(binaryPath);
  auto pathToBuiltWithFile = fs::current_path() / pathOutput / "built_with.txt";
  auto oldHash = WStringToString(readFile(pathToBuiltWithFile));

  if (flagRunUserBuild == false || !binExists || oldHash != version_hash) {
    auto r = exec(compileCommand.str());

    if (r.exitCode != 0) {
      log("Unable to build: " + r.output);
      exit(WEXITSTATUS(r.exitCode));
    }
    writeFile(pathToBuiltWithFile, version_hash);

    log("compiled native binary");
  }

  //
  // Linux Packaging
  // ---
  //
  if (flagShouldPackage && platform.linux) {
    fs::path pathSymLinks = {
      pathPackage /
      "usr" /
      "local" /
      "bin"
    };

    auto linuxExecPath = fs::path {
      fs::path("/opt") /
      settings["name"] /
      settings["executable"]
    };

    fs::create_directories(pathSymLinks);
    fs::create_symlink(
      linuxExecPath,
      pathSymLinks / settings["executable"]
    );

    std::stringstream archiveCommand;

    archiveCommand
      << "dpkg-deb --build --root-owner-group "
      << pathPackage.string()
      << " "
      << (target / pathOutput).string();

    auto r = std::system(archiveCommand.str().c_str());

    if (r != 0) {
      log("error: failed to create deb package");
      exit(1);
    }
  }

  //
  // MacOS Code Signing
  // ---
  //
  if (flagCodeSign && platform.mac) {
    //
    // https://www.digicert.com/kb/code-signing/mac-os-codesign-tool.htm
    // https://developer.apple.com/forums/thread/128166
    // https://wiki.lazarus.freepascal.org/Code_Signing_for_macOS
    //
    std::stringstream signCommand;
    std::string entitlements = "";

    if (settings.count("entitlements") == 1) {
      // entitlements = " --entitlements " + (target / settings["entitlements"]).string();
    }

    if (settings.count("mac_sign") == 0) {
      log("'mac_sign' key/value is required");
      exit(1);
    }

    std::stringstream commonFlags;

    commonFlags
      << " --force"
      << " --options runtime"
      << " --timestamp"
      << entitlements
      << " --sign '" + settings["mac_sign"] + "'"
      << " "
    ;

    if (settings["mac_sign_paths"].size() > 0) {
      auto paths = split(settings["mac_sign_paths"], ';');

      for (int i = 0; i < paths.size(); i++) {
        std::string prefix = (i > 0) ? ";" : "";

        signCommand
          << prefix
          << "codesign "
          << commonFlags.str()
          << (pathResources / paths[i]).string()
        ;
      }
      signCommand << ";";
    }

    signCommand
      << "codesign"
      << commonFlags.str()
      << (pathBin / executable).string()

      << "; codesign"
      << commonFlags.str()
      << pathPackage.string();

    log(signCommand.str());
    auto r = exec(signCommand.str());

    if (r.exitCode != 0) {
      log("Unable to sign");
      exit(WEXITSTATUS(r.exitCode));
    }

    log("finished code signing");
  }

  //
  // MacOS Packaging
  // ---
  //
  if (flagShouldPackage && platform.mac) {
    std::stringstream zipCommand;
    auto ext = ".zip";
    auto pathToBuild = target / pathOutput / "build";

    pathToArchive = target / pathOutput / (settings["executable"] + ext);

    zipCommand
      << "ditto"
      << " -c"
      << " -k"
      << " --sequesterRsrc"
      << " --keepParent"
      << " "
      << pathPackage.string()
      << " "
      << pathToArchive.string();

    auto r = std::system(zipCommand.str().c_str());

    if (r != 0) {
      log("error: failed to create zip for notarization");
      exit(1);
    }

    log("created package artifact");
  }

  //
  // MacOS Notorization
  // ---
  //
  if (flagShouldNotarize && platform.mac) {
    std::stringstream notarizeCommand;
    std::string username = getEnv("APPLE_ID");
    std::string password = getEnv("APPLE_ID_PASSWORD");

    notarizeCommand
      << "xcrun"
      << " altool"
      << " --notarize-app"
      << " --username \"" << username << "\""
      << " --password \"" << password << "\""
      << " --primary-bundle-id \"" << settings["bundle_identifier"] << "\""
      << " --file \"" << pathToArchive.string() << "\""
    ;

    auto r = exec(notarizeCommand.str().c_str());

    if (r.exitCode != 0) {
      log("Unable to notarize");
      exit(WEXITSTATUS(r.exitCode));
    }

    std::regex re(R"(\nRequestUUID = (.+?)\n)");
    std::smatch match;
    std::string uuid;

    if (std::regex_search(r.output, match, re)) {
      uuid = match.str(1);
    }

    int requests = 0;

    log("polling for notarization");

    while (!uuid.empty()) {
      if (++requests > 1024) {
        log("apple did not respond to the request for notarization");
        exit(1);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1024 * 6));
      std::stringstream notarizeStatusCommand;

      notarizeStatusCommand
        << "xcrun"
        << " altool"
        << " --notarization-info " << uuid
        << " -u " << username
        << " -p " << password;

      auto r = exec(notarizeStatusCommand.str().c_str());

      std::regex re(R"(\n *Status: (.+?)\n)");
      std::smatch match;
      std::string status;

      if (std::regex_search(r.output, match, re)) {
        status = match.str(1);
      }

      if (status.find("in progress") != -1) {
        log("Checking for updates from apple");
        continue;
      }

      auto lastStatus = r.output;

      if (status.find("invalid") != -1) {
        log("apple rejected the request for notarization");

        log(lastStatus);

        std::stringstream notarizeHistoryCommand;

        notarizeHistoryCommand
          << "xcrun"
          << " altool"
          << " --notarization-history 0"
          << " -u " << username
          << " -p " << password;

        auto r = exec(notarizeHistoryCommand.str().c_str());

        if (r.exitCode != 0) {
          log("Unable to get notarization history");
          exit(WEXITSTATUS(r.exitCode));
        }

        log(r.output);

        exit(1);
      }

      if (status.find("success") != -1) {
        log("successfully notarized");
        break;
      }

      if (status.find("success") == -1) {
        log("apple was unable to notarize");
        break;
      }
    }

    log("finished notarization");
  }

  //
  // Windows Packaging
  // ---
  //
  if (flagShouldPackage && platform.win) {
    #ifdef _WIN32

    auto GetPackageWriter = [&](_In_ LPCWSTR outputFileName, _Outptr_ IAppxPackageWriter** writer) {
      HRESULT hr = S_OK;
      IStream* outputStream = NULL;
      IUri* hashMethod = NULL;
      APPX_PACKAGE_SETTINGS packageSettings = {0};
      IAppxFactory* appxFactory = NULL;

      hr = SHCreateStreamOnFileEx(
        outputFileName,
        STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,
        0, // default file attributes
        TRUE, // create file if it does not exist
        NULL, // no template
        &outputStream
      );

      if (SUCCEEDED(hr)) {
        hr = CreateUri(
          L"http://www.w3.org/2001/04/xmlenc#sha256",
          Uri_CREATE_CANONICALIZE,
          0, // reserved parameter
          &hashMethod
        );
      }

      if (SUCCEEDED(hr)) {
          packageSettings.forceZip32 = TRUE;
          packageSettings.hashMethod = hashMethod;
      }

      // Create a new Appx factory
      if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(
          __uuidof(AppxFactory),
          NULL,
          CLSCTX_INPROC_SERVER,
          __uuidof(IAppxFactory),
          (LPVOID*)(&appxFactory)
        );
      }

      // Create a new package writer using the factory
      if (SUCCEEDED(hr)) {
        hr = appxFactory->CreatePackageWriter(
          outputStream,
          &packageSettings,
          writer
        );
      }

      // Clean up allocated resources
      if (appxFactory != NULL) {
        appxFactory->Release();
        appxFactory = NULL;
      }

      if (hashMethod != NULL) {
        hashMethod->Release();
        hashMethod = NULL;
      }

      if (outputStream != NULL) {
        outputStream->Release();
        outputStream = NULL;
      }

      return hr;
    };


    std::wstring appx(StringToWString(pathPackage.string()) + L".appx");

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr)) {
      IAppxPackageWriter* packageWriter = NULL;

      hr = GetPackageWriter(appx.c_str(), &packageWriter);

      std::function<void(LPCWSTR, fs::path)> addFiles = [&](auto basePath, auto last) {
        for (const auto & entry : fs::directory_iterator(basePath)) {
          auto p = entry.path().filename().string();

          LPWSTR mime = 0;
          FindMimeFromData(NULL, entry.path().c_str(), NULL, 0, NULL, 0, &mime, 0);

          if (p.find("AppxManifest.xml") == 0) {
            continue;
          }

          if (fs::is_directory(entry.path())) {
            addFiles(entry.path().c_str(), fs::path { last / entry.path().filename() });
            continue;
          }

          auto composite = (fs::path { last / entry.path().filename() });

          if (!mime) {
            mime = (LPWSTR) L"application/octet-stream";
          }

          IStream* fileStream = NULL;
          hr = SHCreateStreamOnFileEx(
            entry.path().c_str(),
            STGM_READ | STGM_SHARE_EXCLUSIVE,
            0,
            FALSE,
            NULL,
            &fileStream
          );

          if (SUCCEEDED(hr)) {
            auto hr2 = packageWriter->AddPayloadFile(
              composite.c_str(),
              mime,
              APPX_COMPRESSION_OPTION_NONE,
              fileStream
            );

            if (SUCCEEDED(hr2)) {
            } else {
              log("mimetype?: " + WStringToString(mime));
              log("Could not add payload file: " + entry.path().string());
            }
          } else {
            log("Could not add file: " + entry.path().string());
          }

          if (fileStream != NULL) {
            fileStream->Release();
            fileStream = NULL;
          }
        }
      };

      addFiles(pathPackage.c_str(), fs::path {});

      IStream* manifestStream = NULL;

      if (SUCCEEDED(hr)) {
        auto p = (fs::path { pathPackage / "AppxManifest.xml" });

        hr = SHCreateStreamOnFileEx(
          p.c_str(),
          STGM_READ | STGM_SHARE_EXCLUSIVE,
          0,
          FALSE,
          NULL,
          &manifestStream
        );
      } else {
        log("Could not get package writer or add files");
      }

      if (SUCCEEDED(hr)) {
        hr = packageWriter->Close(manifestStream);
      } else {
        log("Could not generate AppxManifest.xml");
        // log("Run MakeAppx.exe manually to see more errors");
      }

      if (manifestStream != NULL) {
        manifestStream->Release();
        manifestStream = NULL;
      }
      if (packageWriter != NULL) {
        packageWriter->Release();
        packageWriter = NULL;
      }

      CoUninitialize();
    } else {
      log("Unable to initialize package writer");
    }

    if (SUCCEEDED(hr)) {
      log("Package saved");
    }
    else {
      _com_error err(hr);
      std::string msg = std::string( err.ErrorMessage() );
      log("Unable to save package; " + msg);

      exit(1);
    }

    #endif
  }


  //
  // Windows Code Signing
  //
  if (flagCodeSign && platform.win) {
    //
    // https://www.digicert.com/kb/code-signing/signcode-signtool-command-line.htm
    //
    auto sdkRoot = fs::path("C:\\Program Files (x86)\\Windows Kits\\10\\bin");
    auto pathToSignTool = getEnv("SIGNTOOL");

    if (pathToSignTool.size() == 0) {
      // TODO assumes last dir that contains dot. posix doesnt guarantee
      // order, maybe windows does, but this should probably be smarter.
      for (const auto& entry : fs::directory_iterator(sdkRoot)) {
        auto p = entry.path().string();
        if (p.find(".") != -1) pathToSignTool = p;
      }

      pathToSignTool = fs::path {
        fs::path(pathToSignTool) /
        "x64" /
        "signtool.exe"
      }.string();
    }

    if (pathToSignTool.size() == 0) {
      log("WARNING! Can't find windows 10 SDK, assuming signtool.exe is in the path.");
      pathToSignTool = "signtool.exe";
    }

    std::stringstream signCommand;
    std::string password = getEnv("CSC_KEY_PASSWORD");

    if (password.size() == 0) {
      log("ERROR! Environment variable 'CSC_KEY_PASSWORD' is empty!");
      exit(1);
    }

    signCommand
      << "\""
      << "\"" << pathToSignTool << "\""
      << " sign"
      << " /debug"
      << " /f " << settings["win_pfx"]
      << " /p \"" << password << "\""
      << " /v"
      << " /tr http://timestamp.digicert.com"
      << " /td sha256"
      << " /fd sha256"
      << " " << pathPackage.string() << ".appx"
      << "\"";

      log(signCommand.str());
      auto r = exec(signCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to sign");
        log(pathPackage.string());
        log(r.output);
        exit(WEXITSTATUS(r.exitCode));
      }
  }

  int exitCode = 0;
  if (flagShouldRun) {
    std::string execName = "";
    if (platform.win) {
      execName = (settings["executable"] + ".exe");
    } else {
      execName = (settings["executable"]);
    }

    auto cmd = (pathBin / execName).string();

    auto runner = trim(std::string(STR_VALUE(CMD_RUNNER)));
    auto prefix = runner.size() > 0 ? runner + std::string(" ") : runner;
    auto code = std::system((prefix + cmd + argvForward.str()).c_str());

    // TODO: What kind of exit code does std::system give on windows
    exitCode = code > 0 ? WEXITSTATUS(code) : code;
  }

  return exitCode;
}
