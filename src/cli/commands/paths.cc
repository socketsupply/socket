#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  Paths::Paths (Program& program) : Command("paths") {
    this->option(Option {
      "--platform",
      "-P",
      Option::Type::Single,
      Option::Requirement::Optional,
      Option::Expected {{
        "android",
        "android-emulator",
        "desktop",
        "ios",
        "ios-simulator",
        "linux",
        "mac",
        "win",
        "windows"
      }}
    });

    this->option(Option {
      "--path",
      "",
      Option::Type::Multiple,
      Option::Requirement::Optional,
      Option::Expected {{
        "archive",
        "bin",
        "output",
        "package",
        "resources"
      }}
    });

    this->option(Option {
      "--format",
      "-f",
      Option::Type::Single,
      Option::Requirement::Optional,
      Option::Expected {{
        "default",
        "json"
      }}
    });

    this->callback = [&program](const Command& command, const Options& options) {
      Vector<String> queries;
      String targetPlatform = "desktop";
      String targetFormat = "default";
      String targetPath;
      Map output;

      for (const auto& option : options) {
        if (option.isPositional() && targetPath.size() == 0) {
          targetPath = option.value();
        }

        if (option == "--platform") {
          targetPlatform = option.value();
        }

        if (option == "--path") {
          queries = option.values();
        }

        if (option == "--format") {
          targetFormat = option.value();
        }
      }

      if (targetPlatform == "desktop") {
        targetPlatform = platform.os;
      }

      if (targetPath.size() == 0) {
        targetPath = fs::current_path().string();
      } else {
        targetPath = fs::absolute(targetPath).lexically_normal().string();
      }

      if (!fs::exists(targetPath)) {
        program.logger.error(
          format("Target path does not exist: '{}'", targetPath)
        );
        return false;
      }

      fs::current_path(targetPath);

      if (!program.preloadUserConfig(targetPath)) {
        return false;
      }

      const auto paths = program.getPaths(targetPath, targetPlatform);

      if (queries.size() == 0) {
        queries.push_back("archive");
        queries.push_back("bin");
        queries.push_back("output");
        queries.push_back("package");
        queries.push_back("resources");
      }

      for (const auto& query : queries) {
        String value;

        if (query == "archive") value = paths.archive.string();
        else if (query == "bin") value = paths.bin.string();
        else if (query == "output") value = paths.output.string();
        else if (query == "package") value = paths.package.string();
        else if (query == "resources") value = paths.resources.string();

        if (value.size() > 0) {
          output[query] = value;
        }
      }

      if (targetFormat == "json") {
        JSON::Object json = output;
        IO::write(json.str());
      } else {
        int maxKeyWidth = 0;
        for (const auto& tuple : output) {
          const auto size = tuple.first.size();
          maxKeyWidth = size > maxKeyWidth ? size : maxKeyWidth;
        }

        for (const auto& tuple : output) {
          const int n = (tuple.first.size() - maxKeyWidth) + 1;
          const auto padding = String((maxKeyWidth - tuple.first.size() ) + 1, ' ');
          if (tuple.second.size() > 0) {
            IO::write(format("{}:{}\t{}", tuple.first, padding, tuple.second));
          }
        }
      }

      return true;
    };
  }
}
