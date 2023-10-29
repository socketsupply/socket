#ifndef SSC_CLI_ANDROID_H
#define SSC_CLI_ANDROID_H

#include "cli.hh"

namespace SSC::CLI {
  const auto ANDROID_AVD_DEVICE_NAME = String("SSCAVD");
  const auto ANDROID_PLATFORM_VERSION = String("34");
  const auto ANDROID_NDK_VERSION = String("26.0.10792818");
  const auto ANDROID_PLATFORM = String("android-") + ANDROID_PLATFORM_VERSION;

  constexpr int ANDROID_TASK_SLEEP_TIME = 200;
  constexpr int ANDROID_TASK_TIMEOUT = 120000;

  class Android {
    public:
      struct Device {
        enum class Type {
          Emulator,
          Tablet,
          Phone,
          TV
        };

        String name;
        String status; // "device", "offline", "unauthorized"
        String product;
        String model;
        String device;
        Type type = Type::Phone;

        Device () = default;
        Device (const String& name);
        const String str () const;
        const JSON::Object json () const;

        bool isEmulator () const;
        bool isTablet () const;
        bool isPhone () const;
        bool isTV () const;
      };

      class Artifacts {
        public:
          enum class Type {
            DevDebug,
            DevReleaseSigned,
            DevReleaseUnsigned,
            DevRelease = DevReleaseUnsigned,
            LiveDebug,
            LiveReleaseSigned,
            LiveReleaseUnsigned,
            LiveRelease = LiveReleaseUnsigned
          };

          Android& android;
          Artifacts (Android&);

          const String apk (const Path& path, const Type = Type::DevDebug) const;
          const String bundle (const Path& path, const Type = Type::DevDebug) const;
      };

      class Application {
        public:
          enum class Target { Any, Device, Emulator };

          Android& android;
          Application (Android&);
          bool start (const Target target = Target::Any);
          bool install (
            const Path& path,
            const Target target = Target::Any,
            const Artifacts::Type type = Artifacts::Type::DevDebug
          );

          bool install (
            const Path& path,
            const Device& device,
            const Artifacts::Type type = Artifacts::Type::DevDebug
          );
      };

      class Emulator {
        public:
          Atomic<bool> isBooting = false;
          Atomic<bool> isRunning = false;
          Android& android;

          Emulator (Android&);
          bool boot ();
      };

      class SDK {
        public:
          Android& android;
          SDK (Android&);

          const ExecOutput adb (
            const String& command,
            const Vector<String> arguments = Vector<String> {},
            bool useSystem = false
          ) const;

          const ExecOutput avdmanager (
            const String& command,
            const Vector<String> arguments = Vector<String> {},
            bool useSystem = false
          ) const;

          const ExecOutput sdkmanager (
            const String& command,
            const Vector<String> arguments = Vector<String> {},
            bool useSystem = false
          ) const;

          bool setup () const;
      };

      Application application;
      Artifacts artifacts;
      Emulator emulator;
      SDK sdk;

      Android ();

      Vector<Device> devices ();
  };
}
#endif
