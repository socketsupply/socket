#include <fstream>

#include "extension.hh"
#include "../process/process.hh"

namespace SSC::CLI {
  inline String resolveConfigPath (const Path& path) {
    if (!fs::exists(path)) {
      return "";
    }

    if (fs::is_directory(path)) {
      return path / "socket.ini";
    }

    return path;
  }

  inline String readConfig (const Path& path) {
    if (!fs::exists(path)) {
      return "";
    }

    if (fs::is_directory(path)) {
      return readConfig(path / "socket.ini");
    }

    auto input = InputFileStream(path.string());
    auto output = StringStream();
    input << output.rdbuf();
    input.close();
    return output.str();
  }

  Extension::Extension (const ExtensionConfig& config) : config(config) {
  }

  Extension::Extension (const String& source) {
    auto data = readConfig(source);

    if (data.size() > 0) {
      this->config = ExtensionConfig(data);
    }

    this->config.set("source", source);
  }

  const String Extension::source () const {
    return this->config.source();
  }

  const String Extension::path () const {
    return this->config.path();
  }

  bool Extension::maybeInstall () {
    return false;
  }

  bool Extension::install () {
    return false;
  }
}
