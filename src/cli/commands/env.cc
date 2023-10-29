#include "../cli.hh"
#include "../io.hh"

static constexpr auto HELP  = R"TEXT(
ssc v{{ssc.version}}

Print useful environment variables.

usage:
  ssc env
)TEXT";

namespace SSC::CLI::commands {
  Env::Env (Program& program) : Command("env", HELP) {
    this->callback = [&program](const Command& command, const Options& options) {
      const auto positionals = Command::Option::getPositionals(options);
      const auto targetPath = (
        positionals.size() > 0
          ? Path(positionals[0].str())
          : fs::current_path()
      );

      // don't validate and return early, just warn
      program.preloadUserConfig(targetPath);

      auto program = Program::instance();
      auto envs = Map();

      envs["DEBUG"] = SSC::Env::get("DEBUG");
      envs["VERBOSE"] = SSC::Env::get("VERBOSE");

      // runtime variables
      envs["SOCKET_HOME"] = program->env.SOCKET_HOME();
      envs["SOCKET_HOME_API"] = SSC::Env::get("SOCKET_HOME_API");

      // platform OS variables
      envs["PWD"] = SSC::Env::get("PWD");
      envs["HOME"] = SSC::Env::get("HOME");
      envs["LANG"] = SSC::Env::get("LANG");
      envs["USER"] = SSC::Env::get("USER");
      envs["SHELL"] = SSC::Env::get("SHELL");
      envs["HOMEPATH"] = SSC::Env::get("HOMEPATH");
      envs["LOCALAPPDATA"] = SSC::Env::get("LOCALAPPDATA");
      envs["XDG_DATA_HOME"] = SSC::Env::get("XDG_DATA_HOME");

      // compiler variables
      envs["CXX"] = SSC::Env::get("CXX");
      envs["PREFIX"] = SSC::Env::get("PREFIX");
      envs["CXXFLAGS"] = SSC::Env::get("CXXFLAGS");

      // locale variables
      envs["LC_ALL"] = SSC::Env::get("LC_ALL");
      envs["LC_CTYPE"] = SSC::Env::get("LC_CTYPE");
      envs["LC_TERMINAL"] = SSC::Env::get("LC_TERMINAL");
      envs["LC_TERMINAL_VERSION"] = SSC::Env::get("LC_TERMINAL_VERSION");

      // platform dependency variables
      envs["JAVA_HOME"] = SSC::Env::get("JAVA_HOME");
      envs["GRADLE_HOME"] = SSC::Env::get("GRADLE_HOME");
      envs["ANDROID_HOME"] = program->env.ANDROID_HOME();
      envs["ANDROID_SUPPORTED_ABIS"] = SSC::Env::get("ANDROID_SUPPORTED_ABIS");

      // apple specific platform variables
      envs["APPLE_ID"] = SSC::Env::get("APPLE_ID");
      envs["APPLE_ID_PASSWORD"] = SSC::Env::get("APPLE_ID");

      // windows specific platform variables
      envs["SIGNTOOL"] = SSC::Env::get("SIGNTOOL");
      envs["WIN_DEBUG_LIBS"] = SSC::Env::get("WIN_DEBUG_LIBS");
      envs["CSC_KEY_PASSWORD"] = SSC::Env::get("CSC_KEY_PASSWORD");

      // ssc variables
      envs["SSC_CI"] = SSC::Env::get("SSC_CI");
      envs["SSC_RC"] = SSC::Env::get("SSC_RC");
      envs["SSC_RC_FILENAME"] = SSC::Env::get("SSC_RC_FILENAME");
      envs["SSC_ENV_FILENAME"] = SSC::Env::get("SSC_ENV_FILENAME");

      if (envs["SOCKET_HOME_API"].size() == 0) {
        envs["SOCKET_HOME_API"] = trim(program->env.SOCKET_HOME() + "api");
      }

      // gather all evironemnt variables relevant to project in the `socket.ini`
      // file, which may overload the variables above
      for (const auto& entry : program->userConfig.slice("env")) {
        auto& key = entry.first;
        auto value = trim(entry.second);

        if (value.size() == 0) {
          value = trim(SSC::Env::get(key));
        }

        envs[key] = value;
      }

      for (const auto& key : program->userConfig.list("env")) {
        auto value = trim(SSC::Env::get(key));
        envs[key] = value;
      }

      // gather all evironemnt variables relevant to local `.sscrc` files
      // file, which may overload the variables above
      for (const auto& entry : program->rc.slice("env")) {
        auto& key = entry.first;
        auto value = trim(entry.second);

        if (value.size() == 0) {
          value = trim(SSC::Env::get(key));
        }

        envs[key] = value;
      }

      for (const auto& key : program->rc.list("env")) {
        auto value = trim(SSC::Env::get(key));
        envs[key] = value;
      }

      // print all environment variables that have values
      for (const auto& entry : envs) {
        auto& key = entry.first;
        auto value = trim(entry.second);

        if (value.size() == 0) {
          value = trim(SSC::Env::get(key));
        }

        if (value.size() > 0) {
          std::cout << key << "=" << value << std::endl;
        }
      }
      return true;
    };
  }
}
