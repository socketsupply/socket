#include "common.hh"
#include "cli.hh"
#include "process.hh"
#include <filesystem>

#ifdef _WIN32
#include <shlwapi.h>
#include <strsafe.h>
#include <comdef.h>
#include <AppxPackaging.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Urlmon.lib")
#endif

#ifndef CMD_RUNNER
#define CMD_RUNNER
#endif

using namespace SSC;
using namespace std::chrono;

auto start = std::chrono::system_clock::now();
bool porcelain = false;

void log (const std::string s) {
  if (porcelain || s.size() == 0) return;

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
  attrs["node_platform"] = platform.arch == "arm64" ? "arm64" : "x64";
  auto cwd = fs::current_path();
  fs::create_directories(cwd / "src");
  SSC::writeFile(cwd / "src" / "index.html", "<html>Hello, World</html>");
  SSC::writeFile(cwd / "ssc.config", tmpl(gDefaultConfig, attrs));
}

static std::string getCxxFlags() {
  auto flags = getEnv("CXX_FLAGS");
  return flags.size() > 0 ? " " + flags : "";
}

void printHelp () {
  std::cout << gHelpText << std::endl;
}

int main (const int argc, const char* argv[]) {
  Map attrs;
  attrs["ssc_version"] = SSC::version;

  if (argc < 2) {
    printHelp();
    exit(0);
  }

  auto is = [](const std::string& s, const auto& p) -> bool {
    return s.compare(p) == 0;
  };

  auto const subcommand = argv[1];

  if (is(subcommand, "-v")) {
    std::cout << SSC::full_version << std::endl;
    exit(0);
  }

  if (is(subcommand, "-h")) {
    printHelp();
    exit(0);
  }

  if (is(subcommand, "init")) {
    init(attrs);
    exit(0);
  }

  if (is(subcommand, "mobiledeviceid")) {
    if (platform.os != "mac") {
      log("mobiledeviceid is only supported on macOS.");
      exit(0);
    } else {
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
  }

  if (is(subcommand, "install-app")) {
    if (platform.os != "mac") {
      log("install-app is only supported on macOS.");
      exit(0);
    } else {
      const bool hasCfgUtilInPath = exec("type cfgutil").exitCode == 0;
      const bool hasCfgUtil = fs::exists("/Applications/Apple Configurator.app/Contents/MacOS/cfgutil");
      if (!hasCfgUtilInPath || !hasCfgUtil) {
        log("Please install Apple Configurator from https://apps.apple.com/us/app/apple-configurator/id1037126344");
        exit(1);
      } else if (!hasCfgUtilInPath) {
        log("We highly recommend to install Automation Tools from the Apple Configurator main menu.");
      }
      if (argc < 4) {
        std::cout << "usage: ssc install-app --target=<target> <app-path>" << std::endl;
        exit(1);
      }
      if (std::string(argv[2]).find("--target=") == 0) {
        auto target = std::string(argv[2]).substr(9);
        // TODO: add Android support
        if (target != "ios") {
          std::cout << "Unsupported target: " << target << std::endl;
          exit(1); 
        }
      } else {
        std::cout << "usage: ssc install-app --target=<target> <app-path>" << std::endl;
        exit(1);
      }
      auto path = argv[3];
      auto target = fs::path(path);
      if (path[0] == '.') {
        target = fs::absolute(target);
      }
      auto configPath = target / "ssc.config";
      auto _settings = WStringToString(readFile(configPath));
      auto settings = parseConfig(_settings);
      auto ipaPath = (
        target /
        settings["output"] /
        "build" /
        std::string(settings["name"] + ".ipa") /
        std::string(settings["name"] + ".ipa")
      );
      if (!fs::exists(ipaPath)) {
        log("Could not find " + ipaPath.string());
        exit(1);
      }
      // TODO: this command will install the app to all connected device which were added to the provisioning profile
      //       we should add a --ecid option support to specify the device
      auto command = hasCfgUtilInPath
        ? "cfgutil install-app " + std::string(ipaPath)
        : "/Applications/Apple\\ Configurator.app/Contents/MacOS/cfgutil install-app " + std::string(ipaPath);
      auto r = exec(command);
      if (r.exitCode != 0) {
        log("failed to install the app. Is the device plugged in?");
        exit(1);
      }
      log("successfully installed the app on your device(s).");
      exit(0);
    }
  }

  auto const shouldPrintBuildTarget = is(subcommand, "print-build-target");

  if (is(subcommand, "compile") || shouldPrintBuildTarget) {
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
    bool flagBuildForAndroidEmulator = false;
    bool flagBuildForSimulator = false;
    bool flagPrintBuildPath = false;

    if (shouldPrintBuildTarget) {
      porcelain = true;
      flagPrintBuildPath = true;
    }

    std::string devPort("0");
    auto cnt = 0;

    for (auto const arg : std::span(argv, argc).subspan(2, argc-3)) {
      if (is(arg, "-c")) {
        flagCodeSign = true;
      }

      if (is(arg, "-e")) {
        if (platform.os != "mac") {
          log("-e is only supported on macOS. Ignoring.");
        } else {
          // TODO: we don't use it
          flagEntitlements = true;
        }
      }

      if (is(arg, "-n")) {
        if (platform.os != "mac") {
          log("-n is only supported on macOS. Ignoring.");
        } else {
          flagShouldNotarize = true;
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

      if (is(arg, "--prod")) {
        flagDebugMode = false;
      }

      if (std::string(arg).find("--target=") == 0) {
        auto target = std::string(arg).substr(9);
        if (target == "ios") {
          flagBuildForIOS = true;
        } else if (target == "android") {
          flagBuildForAndroid = true;
        } else if (target == "androidemulator") {
          flagBuildForAndroid = true;
          flagBuildForAndroidEmulator = true;
        } else if (target == "iossimulator") {
          flagBuildForIOS = true;
          flagBuildForSimulator = true;
        }
      }

      if (is(arg, "--test")) {
        flagTestMode = true;
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

    auto const path = argv[argc-1];
    auto target = fs::absolute(fs::path(path));

    if (path[0] == '-') {
      log("a directory was expected as the last argument");
      exit(1);
    }

    if (path[0] == '.') {
      target = fs::absolute(target);
    }

    if (std::string(target).back() == '.') {
      target = fs::path(std::string(target).substr(0, std::string(target).size() - 1));
    }

    auto configPath = target / "ssc.config";

    if (!fs::exists(configPath)) {
      porcelain = false;
      log("ssc.config not found in " + target.string());
      exit(1);
    }

    auto _settings = WStringToString(readFile(configPath));
    auto settings = parseConfig(_settings);

    if (settings.count("revision") == 0) {
      settings["revision"] = "1";
    }

    const std::vector<std::string> required = {
      "name",
      "title",
      "executable",
      "version"
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

    std::replace(settings["name"].begin(), settings["name"].end(), ' ', '_');
    settings["name"] += suffix;
    settings["title"] += suffix;
    settings["executable"] += suffix;

    auto pathOutput = settings["output"].size() ? settings["output"] : "dist";

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
    if (platform.mac && !flagBuildForIOS && !flagBuildForAndroid) {
      packageName = fs::path(std::string(settings["name"] + ".app"));
      pathPackage = { target / pathOutput / packageName };

      log("preparing build for mac");

      flags = "-std=c++2a -ObjC++";
      flags += " -framework UniformTypeIdentifiers";
      flags += " -framework CoreBluetooth";
      flags += " -framework UserNotifications";
      flags += " -framework WebKit";
      flags += " -framework Cocoa";
      flags += " -DMACOS=1";
      flags += " -Wno-nullability-completeness"; // ignore swift compat
      flags += " -I" + prefixFile();
      flags += " -I" + prefixFile("include");
      flags += " -L" + prefixFile("lib");
      flags += " -luv";
      flags += " " + getCxxFlags();

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

      Map options = {{ "full_version", std::string(SSC::full_version) }};
      auto credits = tmpl(gCredits, options);

      writeFile(pathResourcesRelativeToUserBuild / "Credits.html", credits);
    }

    if (flagBuildForAndroid) {
      auto bundle_identifier = settings["bundle_identifier"];
      auto bundle_path = fs::path(replace(bundle_identifier, "\\.", "/")).make_preferred();
      auto bundle_path_underscored = replace(bundle_identifier, "\\.", "_");

      auto output = fs::absolute(target) / settings["output"] / "android";
      auto app = output / "app";
      auto src = app / "src";
      auto cpp = src / "main" / "cpp";
      auto res = src / "main" / "res";
      auto pkg = src / "main" / "java" / bundle_path;

      // clean and create output directories
      fs::remove_all(output);
      fs::create_directories(output);
      fs::create_directories(src);
      fs::create_directories(cpp);
      fs::create_directories(res);
      fs::create_directories(pkg);

      // set current path to output directory
      fs::current_path(output);

      std::stringstream gradleInitCommand;
      gradleInitCommand
        << "echo 1 |"
        << "gradle "
        << "--no-configuration-cache "
        << "--no-build-cache "
        << "--no-scan "
        << "--offline "
        << "--quiet "
        << "init";

      if (std::system(gradleInitCommand.str().c_str()) != 0) {
        log("error: failed to invoke `gradle init` command");
        exit(1);
      }

      //writeFile();

      // Core
      fs::copy(trim(prefixFile("src/core.hh")), cpp);
      fs::copy(trim(prefixFile("src/common.hh")), cpp);
      fs::copy(trim(prefixFile("src/preload.hh")), cpp);

      // Android Project
      writeFile(src / "main" / "AndroidManifest.xml", tmpl(gAndroidManifest, settings));
      writeFile(app / "proguard-rules.pro", tmpl(gProGuardRules, settings));
      writeFile(app / "build.gradle", tmpl(gGradleBuildForSource, settings));
      writeFile(output / "settings.gradle", tmpl(gGradleSettings, settings));
      writeFile(output / "build.gradle", tmpl(gGradleBuild, settings));

      // JNI/NDK
      fs::copy(trim(prefixFile("src/android.cc")), cpp);
      writeFile(
        cpp / "android.hh",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android.hh")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_path_underscored
        )
      );

      // Android Source
      writeFile(
        pkg / "main.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      std::stringstream gradleWrapperCommand;

      if (flagDebugMode) {
        gradleWrapperCommand
          << "./gradlew :app:bundleDebug";
      } else {
        gradleWrapperCommand
          << "./gradlew :app:bundle";
      }

      if (std::system(gradleWrapperCommand.str().c_str()) != 0) {
        log("error: failed to invoke `gradlew` command");
        exit(1);
      }

      return 0;
    }

    if (flagBuildForIOS && !platform.mac) {
      log("Building for iOS on a non-mac platform is unsupported");
      exit(1);
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

      if (!flagBuildForSimulator) {
        if (!fs::exists(pathToProject)) {
          log("provisioning profile not found: " + pathToProfile.string() + ". " +
              "Please specify a valid provisioning profile in the " +
              "ios_provisioning_profile field in your ssc.config");
          exit(1);
        }
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
      }

      fs::copy(
        fs::path(prefixFile()) / "lib",
        target / pathOutput / "lib",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      fs::copy(
        fs::path(prefixFile()) / "include",
        target / pathOutput / "include",
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
        "amd64"
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

    if (settings.count("build") != 0) {
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

      // log(buildCommand.str());
      auto r = exec(buildCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to run user build command");
        log(r.output);
        exit(r.exitCode);
      }

      log(r.output);
      log("ran user build command");

      fs::current_path(oldCwd);
    } else {
      fs::path pathInput = settings["input"].size() > 0
        ? target / settings["input"]
        : target / "src";
      fs::copy(
        pathInput,
        pathResourcesRelativeToUserBuild
      );
    }

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
      fs::copy(trim(prefixFile("src/apple.mm")), pathToDist);
      fs::copy(trim(prefixFile("src/common.hh")), pathToDist);
      fs::copy(trim(prefixFile("src/core.hh")), pathToDist);
      fs::copy(trim(prefixFile("src/preload.hh")), pathToDist);

      auto pathBase = pathToDist / "Base.lproj";
      fs::create_directories(pathBase);

      writeFile(pathBase / "LaunchScreen.storyboard", gStoryboardLaunchScreen);
      // TODO allow the user to copy their own if they have one
      writeFile(pathToDist / "socket.entitlements", gXcodeEntitlements);

      //
      // For iOS we're going to bail early and let XCode infrastructure handle
      // building, signing, bundling, archiving, noterizing, and uploading.
      //
      std::stringstream archiveCommand;
      std::string destination = "generic/platform=iOS";
      std::string deviceType;

      if (flagBuildForSimulator) {
        destination = "platform=iOS Simulator,OS=latest,name=" + settings["ios_simulator_device"];

        std::stringstream listDeviceTypesCommand;
        listDeviceTypesCommand
          << "xcrun"
          << " simctl"
          << " list devicetypes";

        auto rListDeviceTypes = exec(listDeviceTypesCommand.str().c_str());
        if (rListDeviceTypes.exitCode != 0) {
          log("failed to list device types using \"" + listDeviceTypesCommand.str() + "\"");
          if (rListDeviceTypes.output.size() > 0) {
            log(rListDeviceTypes.output);
          }
          exit(rListDeviceTypes.exitCode);
        }

        std::regex reDeviceType(settings["ios_simulator_device"] + "\\s\\((com.apple.CoreSimulator.SimDeviceType.(?:.+))\\)");
        std::smatch match;

        if (std::regex_search(rListDeviceTypes.output, match, reDeviceType)) {
          deviceType = match.str(1);
          log("simulator device type: " + deviceType);
        } else {
          auto rListDevices = exec("xcrun simctl list devicetypes | grep iPhone");
          log("failed to find device type: " + settings["ios_simulator_device"] + ". Please provide correct device name for the \"ios_simulator_device\". The list of available devices:\n" + rListDevices.output);
          if (rListDevices.output.size() > 0) {
            log(rListDevices.output);
          }
          exit(rListDevices.exitCode);
        }
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
        log(rArchive.output);
        fs::current_path(oldCwd);
        exit(1);
      }

      log("created archive");

      if (flagBuildForSimulator) {
        std::stringstream listDevicesCommand;
        listDevicesCommand
          << "xcrun"
          << " simctl"
          << " list devices available";
        auto rListDevices = exec(listDevicesCommand.str().c_str());
        if (rListDevices.exitCode != 0) {
          log("failed to list available devices using \"" + listDevicesCommand.str() + "\"");
          if (rListDevices.output.size() > 0) {
            log(rListDevices.output);
          }
          exit(rListDevices.exitCode);
        }

        auto iosSimulatorDeviceSuffix = settings["ios_simulator_device"];
        std::replace(iosSimulatorDeviceSuffix.begin(), iosSimulatorDeviceSuffix.end(), ' ', '_');
        std::regex reSocketSDKDevice("SocketSDKSimulator_" + iosSimulatorDeviceSuffix + "\\s\\((.+)\\)\\s\\((.+)\\)");
        std::smatch match;

        std::string uuid;
        bool booted = false;

        if (std::regex_search(rListDevices.output, match, reSocketSDKDevice)) {
          uuid = match.str(1);
          booted = match.str(2).find("Booted") != std::string::npos;

          log("found SocketSDK simulator VM for " + settings["ios_simulator_device"] + " with uuid: " + uuid);
          if (booted) {
            log("SocketSDK simulator VM is booted");
          } else {
            log("SocketSDK simulator VM is not booted");
          }
        } else {
          log("creating a new iOS simulator VM for " + settings["ios_simulator_device"]);

          std::stringstream listRuntimesCommand;
          listRuntimesCommand
            << "xcrun"
            << " simctl"
            << " list runtimes available";
          auto rListRuntimes = exec(listRuntimesCommand.str().c_str());
          if (rListRuntimes.exitCode != 0) {
            log("failed to list available runtimes using \"" + listRuntimesCommand.str() + "\"");
            if (rListRuntimes.output.size() > 0) {
              log(rListRuntimes.output);
            }
            exit(rListRuntimes.exitCode);
          }
          auto const runtimes = split(rListRuntimes.output, '\n');
          std::string runtime;
          // TODO: improve iOS version detection
          for (auto it = runtimes.rbegin(); it != runtimes.rend(); ++it) {
            if (it->find("iOS") != std::string::npos) {
              runtime = trim(*it);
              log("found runtime: " + runtime);
              break;
            }
          }

          std::regex reRuntime(R"(com.apple.CoreSimulator.SimRuntime.iOS(?:.*))");
          std::smatch matchRuntime;
          std::string runtimeId;

          if (std::regex_search(runtime, matchRuntime, reRuntime)) {
            runtimeId = matchRuntime.str(0);
          }

          std::stringstream createSimulatorCommand;
          createSimulatorCommand
            << "xcrun simctl"
            << " create SocketSDKSimulator_" + iosSimulatorDeviceSuffix
            << " " << deviceType
            << " " << runtimeId;

          auto rCreateSimulator = exec(createSimulatorCommand.str().c_str());
          if (rCreateSimulator.exitCode != 0) {
            log("unable to create simulator VM");
            if (rCreateSimulator.output.size() > 0) {
              log(rCreateSimulator.output);
            }
            exit(rCreateSimulator.exitCode);
          }
          uuid = rCreateSimulator.output;
        }

        if (!booted) {
          log("booting VM " + uuid);
          std::stringstream bootSimulatorCommand;
          bootSimulatorCommand
            << "xcrun"
            << " simctl boot " << uuid;
          auto rBootSimulator = exec(bootSimulatorCommand.str().c_str());
          if (rBootSimulator.exitCode != 0) {
            log("unable to boot simulator VM with command: " + bootSimulatorCommand.str());
            if (rBootSimulator.output.size() > 0) {
              log(rBootSimulator.output);
            }
            exit(rBootSimulator.exitCode);
          }
        }

        if (flagShouldRun) {
          log("run simulator");

          auto pathToXCode = fs::path("/Applications") / "Xcode.app" / "Contents";
          auto pathToSimulator = pathToXCode / "Developer" / "Applications" / "Simulator.app";

          std::stringstream simulatorCommand;
          simulatorCommand
            << "open "
            << pathToSimulator.string()
            << " --args -CurrentDeviceUDID " << uuid;

          auto rOpenSimulator = exec(simulatorCommand.str().c_str());
          if (rOpenSimulator.exitCode != 0) {
            log("unable to run simulator");
            if (rOpenSimulator.output.size() > 0) {
              log(rOpenSimulator.output);
            }
            exit(rOpenSimulator.exitCode);
          }

          std::stringstream installAppCommand;

          installAppCommand
            << "xcrun"
            << " simctl install booted"
            << " " + settings["name"] + ".app";
          // log(installAppCommand.str());
          log("installed booted VM into simulator");

          auto rInstallApp = exec(installAppCommand.str().c_str());
          if (rInstallApp.exitCode != 0) {
            log("unable to install the app into simulator VM with command: " + installAppCommand.str());
            if (rInstallApp.output.size() > 0) {
              log(rInstallApp.output);
            }
            exit(rInstallApp.exitCode);
          }

          std::stringstream launchAppCommand;
          launchAppCommand
            << "xcrun"
            << " simctl launch --console-pty booted"
            << " " + settings["bundle_identifier"];

          // log(launchAppCommand.str());
          log("launching the app in simulator");

          auto rlaunchApp = exec(launchAppCommand.str().c_str());
          if (rlaunchApp.exitCode != 0) {
            log("unable to launch the app: " + launchAppCommand.str());
            if (rlaunchApp.output.size() > 0) {
              log(rlaunchApp.output);
            }
            exit(rlaunchApp.exitCode);
          }
        }
      }

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

      // TODO(@chicoxyzzy): run on iPhone
      // if (flagShouldRun && !flagRunOnSimulator) {
      // }

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
      << " -DIOS=" << (flagBuildForIOS ? 1 : 0)
      << " -DANDROID=" << (flagBuildForAndroid ? 1 : 0)
      << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
      << " -DPORT=" << devPort
      << " -DSETTINGS=\"" << encodeURIComponent(_settings) << "\""
      << " -DVERSION=" << SSC::version
      << " -DVERSION_HASH=" << SSC::version_hash
    ;

    log(compileCommand.str());

    auto binExists = fs::exists(binaryPath);
    auto pathToBuiltWithFile = fs::current_path() / pathOutput / "built_with.txt";
    auto oldHash = WStringToString(readFile(pathToBuiltWithFile));

    if (flagRunUserBuild == false || !binExists || oldHash != SSC::version_hash) {
      auto r = exec(compileCommand.str());

      if (r.exitCode != 0) {
        log("Unable to build");
        log(r.output);
        exit(r.exitCode);
      }

      log(r.output);

      writeFile(pathToBuiltWithFile, SSC::version_hash);

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
    // MacOS Stripping
    //
    if (platform.mac) {
      std::stringstream stripCommand;
     
      stripCommand 
        << "strip"
        << " " << binaryPath
        << " -u";

      auto r = exec(stripCommand.str().c_str());
      if (r.exitCode != 0) {
        log(r.output);
        exit(r.exitCode);
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

      if (r.output.size() > 0) {
        if (r.exitCode != 0) {
          log("Unable to sign");
          std::cerr << r.output << std::endl;
          exit(r.exitCode);
        } else {
          std::cout << r.output << std::endl;
        }
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
        exit(r.exitCode);
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
            exit(r.exitCode);
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
          exit(r.exitCode);
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
      exitCode = std::system((prefix + cmd + argvForward.str()).c_str());
    }

    return exitCode;
  }

  std::cout << "Error: subcommand 'ssc " << subcommand << "' is not supported.\nFor more information try 'ssc -h'." << std::endl;
  exit(1);
}
