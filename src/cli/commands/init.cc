#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  Init::Init (Program& program) :
    Command("init", gHelpTextInit, Options {
      { "--config", "-C", Option::Type::Boolean},
      { "--name", "-n", Option::Type::Single}
    })
  {
    this->callback = [](const Command& command, const Options& options) {
      auto shouldGenerateConfigOnly = false;
      auto targetPath = fs::current_path();
      auto program = Program::instance();

      for (const auto& option : options) {
        if (option.name == "--name") {
          program->config.set("build.name", option.str());
        }

        if (option.name == "--config") {
          shouldGenerateConfigOnly = true;
        }

        if (option.isPositional()) {
          const auto value = option.str();
          targetPath = fs::absolute(value).lexically_normal();
          break;
        }
      }

      const auto isCurrentPathEmpty = fs::is_empty(targetPath);

      if (fs::exists(targetPath / "socket.ini")) {
        program->logger.warn("socket.ini already exists in " + targetPath.string());
      } else {
        writeFile(targetPath / "socket.ini", tmpl(gDefaultConfig, program->config.data()));
        program->logger.info("socket.ini created in " + targetPath.string());
      }

      if (!shouldGenerateConfigOnly) {
        if (isCurrentPathEmpty) {
          fs::create_directories(targetPath / "src");
          writeFile(targetPath / "src" / "index.html", gHelloWorld);
          program->logger.info("src/index.html created in " + targetPath.string());
        } else {
          program->logger.warn(
            "Current directory was not empty. "
            "Assuming index.html is already in place."
          );
        }

        if (fs::exists(targetPath / ".gitignore")) {
          program->logger.warn(".gitignore already exists in " + targetPath.string());
        } else {
          writeFile(targetPath / ".gitignore", gDefaultGitignore);
          program->logger.info(".gitignore created in " + targetPath.string());
        }
      }

      return true;
    };
  }
}
