#include <span>

#include "android.hh"
#include "cli.hh"
#include "io.hh"

using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace SSC::CLI {
  static const auto DEFAULT_SSC_RC_FILENAME = String(".sscrc");
  static const auto DEFAULT_SSC_ENV_FILENAME = String(".ssc.env");

  // singleton
  static Program* program = nullptr;

  static const Command::Arguments createArguments (
    const char** argv,
    const int argc,
    const int offset = 0
  ) {
    Command::Arguments arguments;

    for (int i = offset; i < argc; ++i) {
      arguments.push_back(String(argv[i]));
    }

    return arguments;
  }

  void initializeRC (Program* program, Path targetPath) {
    static auto SSC_RC_FILENAME = Env::get("SSC_RC_FILENAME");
    static auto SSC_RC = Env::get("SSC_RC");
    static auto filename = SSC_RC_FILENAME.size() > 0
      ? SSC_RC_FILENAME
      : DEFAULT_SSC_RC_FILENAME;

    auto path = SSC_RC.size() > 0
      ? Path(SSC_RC)
      : targetPath / filename;

    // just set to `targetPath` if resolved `path` doesn't exist
    if (!fs::exists(path) && fs::is_regular_file(path)) {
      path = targetPath;
    }

    if (fs::exists(path) && fs::is_regular_file(path)) {
      program->rc.extend(Config(readFile(path)));

      for (const auto& tuple : program->rc) {
        auto key = tuple.first;
        auto value = tuple.second;
        auto valueAsPath = Path(value).make_preferred();

        // convert env value to normalized path if it exists
        if (fs::exists(fs::status(valueAsPath))) {
          value = valueAsPath.string();
        }

        // auto set environment variables
        if (key.starts_with("env.")) {
          key = key.substr(4, key.size() - 4);
          Env::set(key, value);
        }
      }
    }
  }

  const String Program::Environment::HOME () const {
    static String XDG_DATA_HOME = Env::get("XDG_DATA_HOME");
    static String LOCALAPPDATA = Env::get("LOCALAPPDATA");
    static String HOME = Env::get("HOME");

    if (platform.win && HOME.size() == 0) {
      return LOCALAPPDATA;
    }

    return HOME;
  }

  const String Program::Environment::CFLAGS () const {
    static const auto CFLAGS = Env::get("CFLAGS");
    return CFLAGS;
  }

  const String Program::Environment::CXXFLAGS () const {
    static const auto LDFLAGS = Env::get("LDFLAGS");
    return LDFLAGS;
  }

  const String Program::Environment::SOCKET_HOME () const {
    static String XDG_DATA_HOME = Env::get("XDG_DATA_HOME");
    static String LOCALAPPDATA = Env::get("LOCALAPPDATA");
    static String SOCKET_HOME = Env::get("SOCKET_HOME");
    static String HOME = Env::get("HOME");
    static const bool SSC_DEBUG = (
      Env::get("SSC_DEBUG").size() > 0 ||
      Env::get("DEBUG").size() > 0
    );

    static String socketHome = "";
    static String sep = platform.win ? "\\" : "/";

    if (socketHome.size() == 0) {
      if (SOCKET_HOME.size() > 0) {
        if (SOCKET_HOME.back() != sep[0]) {
          socketHome = SOCKET_HOME + sep;
        } else {
          socketHome = SOCKET_HOME;
        }
      } else if (platform.mac || platform.linux) {
        if (XDG_DATA_HOME.size() == 0) {
          socketHome = HOME + "/.local/share/socket/";
        } else {
          if (XDG_DATA_HOME.back() != sep[0]) {
            socketHome = XDG_DATA_HOME + "/socket/";
          } else {
            socketHome = XDG_DATA_HOME + "socket/";
          }
        }
      } else if (platform.win) {
        socketHome = LOCALAPPDATA + "\\Programs\\socketsupply\\";
      }
    }

    if (socketHome.size() > 0) {
    #ifdef _WIN32
      Env::set((String("SOCKET_HOME=") + socketHome).c_str());
    #else
      setenv("SOCKET_HOME", socketHome.c_str(), 1);
    #endif

      if (SSC_DEBUG) {
        if (program != nullptr) {
          program->logger.warn("'SOCKET_HOME' is set to '" + socketHome + "'");
        }
      }
    }

    return socketHome;
  }

  const String Program::Environment::ANDROID_HOME () const {
    static auto androidHome = Env::get("ANDROID_HOME");

    if (androidHome.size() > 0) {
      return androidHome;
    }

    if (!platform.win) {
      auto cmd = String(
        "dirname $(dirname $(readlink $(which sdkmanager 2>/dev/null) 2>/dev/null) 2>/dev/null) 2>/dev/null"
      );

      auto r = exec(cmd);

      if (r.exitCode == 0) {
        androidHome = trim(r.output);
      }
    }

    if (androidHome.size() == 0) {
      if (platform.mac) {
        androidHome = Env::get("HOME") + "/Library/Android/sdk";
      } else if (platform.unix) {
        androidHome = Env::get("HOME") + "/android";
      } else if (platform.win) {
        // TODO
      }
    }

    if (androidHome.size() > 0) {
    #ifdef _WIN32
      Env::set((String("ANDROID_HOME=") + androidHome).c_str());
    #else
      setenv("ANDROID_HOME", androidHome.c_str(), 1);
    #endif

      static const bool SSC_DEBUG = (
        Env::get("SSC_DEBUG").size() > 0 ||
        Env::get("DEBUG").size() > 0
      );

      if (SSC_DEBUG) {
        if (program != nullptr) {
          program->logger.warn("'ANDROID_HOME' is set to '" + androidHome + "'");
        }
      }
    }

    return androidHome;
  }

  Program::Logger::Logger (Program& program)
    : program(program)
  {}

  void Program::Logger::info (const String& string) {
    return this->log("• ", string);
  }

  void Program::Logger::error (const String& string) {
    return this->log("× ERROR: ", string);
  }

  void Program::Logger::warn (const String& string) {
    return this->log("⚠ WARNING: ", string);
  }

  void Program::Logger::write (const String& string) {
    return this->log("", string);
  }

  void Program::Logger::log (const String& prefix, const String& string) {
    if (this->program.state.logger.quiet || string.size() == 0) {
      return;
    }

  #ifdef _WIN32 // unicode console support
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stderr, nullptr, _IOFBF, 1000);
  #endif

    const auto now = system_clock::now();
    const auto delta = duration_cast<milliseconds>(
      now - this->program.state.logger.last
    ).count();

  #ifdef _WIN32
    std::cerr << prefix << string << " " << delta << "ms" << std::endl;
  #else
    std::cerr << prefix << string << " \033[0;32m+" << delta << "ms\033[0m" << std::endl;
  #endif

    this->program.state.logger.last = system_clock::now();
  }

  Program* Program::instance () {
    return program;
  }

  Program::Program () :
    logger(*this),
    Command("ssc", gHelpText, Options {
      {"--debug", "-D"},
      {"--prefix"},
      {"--verbose", "-V"},
      {"--version", "-v"},
    })
  {
    if (SSC::CLI::program == nullptr) {
      SSC::CLI::program = this;
    }

    // initial state
    this->state.logger.last = system_clock::now();

    // program configuration
    this->config.extend(Map {
      { "ssc.version", SSC::VERSION_FULL_STRING }
    });

    // program commands
    this->command(commands::Init(*this));
    this->command(commands::Env(*this));
    this->command(commands::ListDevices(*this));
    this->command(commands::Setup(*this));
    this->command(commands::Paths(*this));
    this->command(commands::PrintBuildDir(*this));
  }

  bool Program::preloadUserConfig () {
    return this->preloadUserConfig(fs::current_path());
  }

  bool Program::preloadUserConfig (const Path& targetPath) {
    const auto configPath = targetPath / "socket.ini";

    if (!this->validateUserConfig(targetPath)) {
      return false;
    }

    this->userConfig = UserConfig(readFile(configPath));

    // default values
    // TODO(@jwerle): move these values to constants or default configuration
    if (!this->userConfig.contains("build.output")) {
      this->userConfig.set("build.output", "build");
    }

    if (!this->userConfig.contains("meta.lang")) {
      this->userConfig.set("meta.lang", "en-us");
    }

    if (!this->userConfig.contains("meta.version")) {
      this->userConfig.set("meta.version", "1.0.0");
    }

    if (!this->userConfig.contains("meta.title")) {
      this->userConfig.set(
        "meta.title",
        this->userConfig.get("build.name")
      );
    }

    // allow for local `.sscrc` '[settings] ...' entries to overload the
    // project's settings in `socket.ini`:
    // [settings.ios]
    // simulator_device = "My local device"
    this->userConfig.extend(rc.slice("settings"));

    if (this->isDebug()) {
      this->userConfig.append("build.name", "-dev");
    }
    return true;
  }

  bool Program::validateUserConfig () {
    return this->validateUserConfig(fs::current_path());
  }

  bool Program::validateUserConfig (const Path& targetPath) {
    const auto configPath = targetPath / "socket.ini";

    if (!fs::exists(configPath)) {
      this->logger.warn(
        "A 'socket.ini' file was not found in " + targetPath.string() + ".\n"
        "Please run 'ssc init' to create one"
      );
      return false;
    }

    const auto config = Config(readFile(configPath));

    // Check if build.name is set
    if (!config.contains("build.name")) {
      this->logger.error("'[build] name' is required in the 'socket.ini' file.");
      return false;
    }

    // Define regular expression to match spaces, and special characters except dash and underscore
    std::regex name_pattern("[^a-zA-Z0-9_\\-]");
    // Check if the name matches the pattern
    if (std::regex_search(config.get("build.name"), name_pattern)) {
      this->logger.error(
        "'[build] name' in the 'socket.ini' file can only contain "
        "alphanumeric characters, dashes, and underscores."
      );
      return false;
    }

    // Define regular expression to match semver format
    // The semver specification is available at https://semver.org/
    // The pre-release and build metadata are not supported
    std::regex semver_pattern("^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)$");
    // Check if the version matches the pattern
    if (!std::regex_match(config.get("meta.version"), semver_pattern)) {
      this->logger.error(
        "'[meta] version' in the 'socket.ini' file must be in semver format. "
        "Please see 'https://semver.org' for more information."
      );
      return false;
    }

    this->state.program.userConfigValidated = true;
    return true;
  }

  void Program::onSignal (int signal) {
    if (state.application.pid && signal == SIGTERM) {
      this->exit(1);
      return;
    }

    if (state.application.pid == 0 && signal == SIGINT) {
      this->exit(signal);
      return;
    }

  #if !defined(_WIN32)
    if (signal == SIGUSR1) {
    #if defined(__APPLE__)
      // TODO
      //checkLogStore = true;
    #endif
      return;
    }

    if (signal == SIGUSR2) {
      this->exit(0);
      return;
    }
  #endif

    if (state.application.process != nullptr) {
      auto pid = state.application.process->getPID();
      state.application.process->kill(pid);
    } else if (state.application.pid > 0) {
    #if !defined(_WIN32)
      kill(state.application.pid, signal);
    #endif
    }

    state.application.process = nullptr;
    state.application.status = signal;
    state.application.pid = 0;

    state.application.mutex.unlock();
  }

  bool Program::isVerbose () const {
    return this->state.program.verbose;
  }

  bool Program::isDebug () const {
    return this->state.program.debug;
  }

  Program::Paths Program::getPaths (
    const Path& targetPath,
    const String& targetPlatform
  ) const {
    static const auto program = Program::instance();
    const auto platformPath = replace(
      replace(targetPlatform, "windows", "win"),
      "win32",
      "win"
    );
    const auto name = userConfig.get("build.name");

    const auto revision = program->userConfig.get("meta.revision", "1");
    const auto version = program->userConfig.get("meta.version");

    Path resources;
    Path package;
    Path archive; // only for android|android-emulator|ios|ios-simulator
    Path build = program->userConfig.get("build.output");
    // ./<build>/<android|android-emulator|ios|ios-simulator|linux|mac|win>
    Path output = targetPath / build / platformPath;
    Path bin; // only for desktop (linux|mac|win[32|dows])

    if (targetPlatform == "mac") {
      const Path contents = "Contents";
      const Path app = name + ".app";
      resources = output / app / contents / "Resources";
      package = output / app;
      bin = package / contents / "MacOS";
    }

    if (targetPlatform == "linux") {
      // we only support this architecture for now
      const auto arch = "amd64";
      // this follows the .deb file naming convention
      const Path app = format("{}_v{}-{}_{}", name, version, revision, arch);
      resources = output / app / "opt" / name;
      package = output / app;
      bin = resources;
    }

    if (targetPlatform == "win32" || targetPlatform == "win" || targetPlatform == "windows") {
      const Path app = format("{}-v{}", name, version);
      resources = output / app;
      package = resources;
      bin = resources;
    }

    if (targetPlatform == "ios" || targetPlatform == "ios-simulator") {
      resources = output / "ui";
      archive = output / "build" / format("{}.ipa/{}.ipa", name, name);
      package = output;
    }

    if (targetPlatform == "android" || targetPlatform == "android-emulator") {
      const Android android;
      const Path app = output / "app";

      archive = android.artifacts.apk(
        targetPath,
        program->isDebug()
          ? Android::Artifacts::Type::DevDebug
          : Android::Artifacts::Type::LiveDebug
      );

      package = app / "src" / "main";
      resources = package / "assets";
    }

    return Paths { archive, bin, output, package, resources };
  }

  int Program::main (const int argc, const char** argv) {
    if (!this->parse(argc, argv)) {
      return 1;
    }

    // initialize environment variables
    this->env.HOME();
    this->env.CFLAGS();
    this->env.CXXFLAGS();
    this->env.SOCKET_HOME();
    this->env.ANDROID_HOME();

    if (Env::get("SSC_VERBOSE").size() > 0 || Env::get("VERBOSE").size() > 0) {
      this->state.program.verbose = true;
    }

    // `/etc/sscrc` (global)
    initializeRC(this, Path("etc")  / "sscrc" );
    // `/etc/ssc.rc` (global)
    initializeRC(this, Path("etc")  / "ssc.rc" );
    // `/etc/ssc/config` (global)
    initializeRC(this, Path("etc")  / "ssc" / "config");
    // `$HOME/.ssc/config` (user)
    initializeRC(this, Path(this->env.HOME())  / ".ssc" / "config");
    // `$HOME/.config/ssc` (user)
    initializeRC(this, Path(this->env.HOME())  / ".config" / "ssc");
    // `$HOME/.sscrc` (user)
    initializeRC(this, this->env.HOME());
    // `$PWD/.sscrc` (local)
    initializeRC(this, fs::current_path());

    return 0;
  }

  bool Program::parse (const int argc, const char** argv) {
    const auto arguments = createArguments(argv, argc, 1);
    const auto options = this->parseOptions(arguments);

    return this->invoke(arguments, options);
  }

  void Program::exit (int status) const {
    ::exit(status);
  }

  bool Program::invoke (const Arguments& arguments, const Options& options) {
    String subCommand;
    int subCommandIndex = 0;

    for (int i = 0; i < options.size(); ++i) {
      const auto& option = options[i];

      if (subCommand.size() == 0) {
        if (option.name == "--help") {
          this->printHelp(std::cout);
          return true;
        }

        if (option.name == "--version") {
          std::cout << SSC::VERSION_FULL_STRING << std::endl;
          std::cerr << "Installation path: " << this->env.SOCKET_HOME() << std::endl;
          return true;
        }

        if (option.name == "--prefix") {
          std::cout << this->env.SOCKET_HOME() << std::endl;
          return true;
        }
      }

      if (option.name == "--verbose") {
        this->state.program.verbose = true;
      }

      if (option.name == "--debug") {
        this->state.program.debug = true;
      }

      if (option.name == "--prod") {
        this->state.program.debug = false;
      }

      if (option.isPositional()) {
        if (subCommand.size() == 0) {
          subCommand = option.str();
          subCommandIndex = option.index;
        }
      }
    }

    if (subCommand.size() > 0) {
      for (auto& tuple : this->commands) {
        auto& command = tuple.second;
        if (command.name == subCommand) {
          const auto subCommandArguments= Arguments(
            arguments.begin() + subCommandIndex + 1,
            arguments.end()
          );

          const auto subCommandOptions = command.parseOptions(subCommandArguments);
          return command.invoke(subCommandArguments, subCommandOptions);
        }
      }

      this->logger.error(
        "'" + subCommand + "'" + " is not a command. See 'ssc --help'"
      );

      return false;
    }

    this->printHelp(std::cerr);
    return false;
  }
}
