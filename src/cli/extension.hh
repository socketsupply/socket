#ifndef SSC_CLI_EXTENSION_H
#define SSC_CLI_EXTENSION_H
#include "../core/config.hh"

namespace SSC::CLI {
  class Extension {
    public:
      ExtensionConfig config;
      Atomic<bool> installed;

      Extension (const ExtensionConfig& config);
      Extension (const String& source);

      const String source () const;
      const String path () const;

      bool install ();
      bool maybeInstall ();
  };
}
#endif
