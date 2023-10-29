#ifndef SSC_CLI_HH
#define SSC_CLI_HH

#include "../process/process.hh"
#include "../core/core.hh"
#include "templates.hh"
#include "project.hh"

#include <signal.h>

#include <chrono>
#include <span>

#if defined(_WIN32)
#define SSC_NULL_DEVICE "NUL"
#else
#define SSC_NULL_DEVICE "/dev/null"
#endif

#define SSC_NULL_DEVICE_REDIRECT " > " SSC_NULL_DEVICE " 2>&1"

namespace SSC::CLI {
  // Forward
  class Command;
  class Program;

  using Commands = std::map<String, Command>;

  /**
   * A container for a command invocation.
   */
  class Command {
    public:
      /**
       * A container for a configured option and its parsed state.
       */
      struct Option {
        enum class Type {
          Boolean,
          Single,
          Multiple
        };

        enum class Requirement {
          Required,
          Optional
        };

        struct Value {
          Vector<String> vector;
          String string;
          bool boolean;

          const String& operator () () const {
            return this->string;
          }
        };

        using Expected = Value;

        static const Vector<Option> getPositionals (const Vector<Option>& options);

        // configuration
        const String name;
        const String alias;
        const Type type = Type::Boolean;
        const Requirement requirement = Requirement::Optional;

        // state
        Value value;
        Expected expected;
        int index = 0;
        bool isFromRestArguments = false;

        Option (const Option& option);

        Option (
          const Type type = Type::Boolean,
          const Requirement requirement = Requirement::Optional,
          const Value expected = Value {}
        );

        Option (
          const String& name,
          const String& alias = "",
          const Type type = Type::Boolean,
          const Requirement requirement = Requirement::Optional,
          const Value expected = Value {}
        );

        bool isMultiple () const;
        bool isBoolean () const;
        bool isSingle () const;

        bool isPositional () const;
        bool isRequired () const;
        bool isOptional () const;
        bool isRest () const;

        const String str () const;
        const Vector<String> values () const;
        const String dump (bool alias = false) const;

        operator bool () const;
        bool operator == (const Option& right) const;
        bool operator != (const Option& right) const;
        bool operator == (const String& right) const;
        bool operator != (const String& right) const;
      };

      /**
       * A span container for command line arguments.
       */
      using Arguments = Vector<String>;

      /**
       * A container for a list of command options.
       */
      using Options = Vector<Option>;

      /**
       * The invocation callback type.
       */
      using InvokeCallback = Function<bool(const Command&, const Options&)>;

      /**
       * A container of sub commands relative to this command.
       */
      Commands commands;

      /**
       * The name of the command.
       */
      const String name;

      /**
       * The command help text.
       */
      const String help;

      /**
       * The command options that should be parsed for this command.
       */
      Options options;

      /**
       * The invocation callback.
       */
      InvokeCallback callback = nullptr;

      /**
       * `Command` class constructor.
       */
      Command (
        const String& name,
        const String& help = "",
        const Options& options = Options{}
      );

      Command& operator = (const Command& other);

      /**
       * Called when the command is invoked by the program. The invocation
       * forwards parsed command line arguments to the command callback.
       */
      virtual bool invoke (const Arguments& arguments, const Options& options);

      /**
       * Creates a sub command for this command by name with an
       * invocation callback.
       *
       * @param command The command to create
       * @return A reference to the command created.
       */
      const Command& command (const Command& command);
      const Command& command (const String& name, const String& help = "");

      /**
       * Creates an option for this command.
       *
       * @param option The option configuration
       * @return A reference to the option created.
       */
      const Option& option (const Option& option);
      const Option& option (
        const String& name,
        const String& alias = "",
        const Option::Type type = Option::Type::Boolean,
        const Option::Requirement requirement = Option::Requirement::Optional,
        const Option::Value expected = Option::Value {}
      );

      /**
       * Parses command line arguments into an option set based on the commands
       * configured options.
       *
       * @param arguments The command line arguments to parse
       * @return The parsed command line options
       */
      const Options parseOptions (const Arguments& arguments) const;

      /**
       * TODO
       */
      void exit (int status) const;
      void printHelp (std::ostream& stream) const;
  };

  /**
   * A container for the top level program: `ssc(1)`.
   */
  class Program : public Command {
    public:
      struct ApplicationState {
        Atomic<Process::id_type> pid = 0;
        Atomic<int> status = -1;
        Process* process = nullptr;
        Mutex mutex;
      };

      struct ProgramState {
        bool userConfigValidated = false;
        bool verbose = false;
        bool debug = true;
      };

      struct LoggerState {
        std::chrono::time_point<std::chrono::system_clock> last;
        bool quiet = false;
      };

      struct AndroidState {
        Atomic<bool> isEmulatorRunning = false;
      };

      struct State {
        ApplicationState application;
        AndroidState android;
        ProgramState program;
        LoggerState logger;
      };

      struct Logger {
        Program& program;
        void info (const String& string);
        void error (const String& string);
        void warn (const String& string);
        void log (const String& prefix, const String& string);
        void write (const String& string);
        Logger (Program& program);
      };

      struct Environment {
        const String HOME () const;
        const String CFLAGS () const;
        const String CXXFLAGS () const;
        const String SOCKET_HOME () const;
        const String ANDROID_HOME () const;
      };

      struct Paths {
        Path archive;
        Path bin;
        Path output;
        Path package;
        Path resources;
      };

      Environment env;
      Logger logger;
      State state;

      // configs
      UserConfig userConfig;
      Config config;
      Config rc;

      static Program* instance ();

      Program ();
      Program (const Program&) = delete;

      // program
      bool parse (const int argc, const char** argv);
      int main (const int argc, const char** argv);
      void exit (int status) const;

      // command
      bool invoke (const Arguments& arguments, const Options& options) override;

      // user configuration
      bool preloadUserConfig ();
      bool preloadUserConfig (const Path& targetPath);
      bool validateUserConfig ();
      bool validateUserConfig (const Path& targetPath);

      // events
      void onSignal (int signal);

      // predicates
      bool isVerbose () const;
      bool isDebug () const;

      Paths getPaths (
        const Path& targetPath = fs::current_path(),
        const String& targetPlatform = platform.os
      ) const;
  };

  namespace commands {
    class Env : public Command { public: Env (Program& program); };
    class Init : public Command { public: Init (Program& program); };
    class InstallApp : public Command { public: InstallApp (Program& program); };
    class ListDevices : public Command { public: ListDevices (Program& program); };
    class Paths : public Command { public: Paths (Program& program); };
    class PrintBuildDir : public Command { public: PrintBuildDir (Program& program); };
    class Run : public Command { public: Run (Program& program); };
    class Setup : public Command { public: Setup (Program& program); };
  }

  inline void notify (int signal) {
  #if !defined(_WIN32)
    static auto ppid = Env::get("SSC_CLI_PID");
    static auto pid = ppid.size() > 0 ? std::stoi(ppid) : 0;
    if (pid > 0) {
      kill(pid, signal);
    }
  #endif
  }

  inline void notify () {
  #if !defined(_WIN32)
    return notify(SIGUSR1);
  #endif
  }
}
#endif
