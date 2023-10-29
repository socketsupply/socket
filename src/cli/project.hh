#ifndef SSC_CLI_PROJECT_H
#define SSC_CLI_PROJECT_H

#include "../core/core.hh"
#include "compiler.hh"

namespace SSC::CLI {
  class Project {
    public:
      using ProjectType = Compiler::PlatformType;

      const ProjectType type;
      const UserConfig config;
      Compiler compiler;

      Project (const ProjectType type, const UserConfig userConfig);

      bool generate () const;
  };
}
#endif
