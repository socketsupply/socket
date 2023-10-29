#include "project.hh"

namespace SSC::CLI {
  Project::Project (const ProjectType type, const UserConfig userConfig)
    : type(type),
      config(userConfig),
      compiler(type)
  {}

  bool Project::generate () const {
    return false;
  }
}
