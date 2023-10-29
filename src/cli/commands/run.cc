#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  Run::Run (Program& program) : Command("run", gHelpTextRun) {
    this->option(Option {
      "--platform",
      "-P",
      Option::Type::Multiple,
      Option::Requirement::Optional,
      Option::Expected {
        { "android", "android-emulator", "ios", "ios-simulator"}
      }
    });

    this->option("--test", "-T", Option::Type::Single);
    this->option("--env", "-E", Option::Type::Multiple);
    this->option("--prod", "-P");
    this->option("--headless", "-H");
    this->option("--verbose", "-V");

    this->callback = [](const Command& command, const Options& options) {
      return true;
    };
  }
}
