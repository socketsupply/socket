#ifndef SSC_CLI_XCODE_H
#define SSC_CLI_XCODE_H

#include "../process/process.hh"
#include "../core/core.hh"

namespace SSC::CLI {
  Path getConfiguratorUtilityCommandPath ();

  class Apple {
    public:
      struct Device {
        enum class Type {
          Simulator,
          Desktop,
          Tablet,
          Phone,
          TV
        };

        String name;
        String identifier;
        String udid;
        String ecid;
        Type type = Type::Desktop;

        Device () = default;
        Device (const String& name);
        const String str () const;
        const JSON::Object json () const;

        bool isSimulator () const;
        bool isDesktop () const;
        bool isTablet () const;
        bool isPhone () const;
        bool isTV () const;
      };

      class Application {
        public:
          enum class Target {
            Desktop,
            iPhone,
            iPhoneSimulator
          };

          struct InstallOptions {
            String destination; // only applies to Target::Desktop
          };

          Apple& apple;
          Application (Apple&);
          bool start (const Target target = Target::Desktop);
          bool install (
            const Path& path,
            const Target target = Target::Desktop,
            const InstallOptions& options = InstallOptions {}
          );

          bool install (
            const Path& path,
            const Device& device,
            const InstallOptions& options = InstallOptions {}
          );
      };

      class Simulator {
        public:
          Atomic<bool> isBooting = false;
          Atomic<bool> isRunning = false;
          Apple& apple;

          Simulator (Apple&);
          bool boot ();
      };

      class SDK {
        public:
          Apple& apple;
          SDK (Apple&);

          const ExecOutput cfgutil (
            const String& command,
            const Vector<String> arguments = Vector<String> {},
            bool useSystem = false
          ) const;

          const ExecOutput simctl (
            const String& command,
            const Vector<String> arguments = Vector<String> {},
            bool useSystem = false
          ) const;

          const ExecOutput installer (
            const String& command,
            const Vector<String> arguments = Vector<String> {},
            bool useSystem = false
          ) const;

          bool setup () const;
      };

      Application application;
      Simulator simulator;
      SDK sdk;

      Apple ();

      Vector<Device> devices ();
  };
}

#endif
