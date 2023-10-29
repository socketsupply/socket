#include "cli.hh"
#include "io.hh"

namespace SSC::CLI {
  String readFile (const Path& path) {
    auto program = Program::instance();
    if (fs::is_directory(path)) {
      if (program != nullptr) {
        program->logger.warn("trying to read a directory as a file: " + path.string());
      }
      return "";
    }

    std::ifstream stream(path.c_str());
    String content;
    auto buffer = std::istreambuf_iterator<char>(stream);
    auto end = std::istreambuf_iterator<char>();
    content.assign(buffer, end);
    stream.close();
    return content;
  }

  void writeFile (const Path& path, const String& string) {
    std::ofstream stream(path.string());
    stream << string;
    stream.close();
  }

  void appendFile (const Path& path, const String& string) {
    std::ofstream stream;
    stream.open(path.string(), std::ios_base::app);
    stream << string;
    stream.close();
  }
}
