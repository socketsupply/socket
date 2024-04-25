#ifndef SSC_CORE_RESOURCE
#define SSC_CORE_RESOURCE

#include "platform.hh"
#include "types.hh"

namespace SSC {
  class Resource {
    public:
      Atomic<bool> accessing = false;
      Resource () = default;
      virtual bool startAccessing () = 0;
      virtual bool stopAccessing () = 0;
  };

  class FileResource : public Resource {
    struct Cache {
      std::shared_ptr<char*> bytes = nullptr;
      size_t size = 0;
    };

    Cache cache;
    std::shared_ptr<char*> bytes = nullptr;
    public:
      static std::map<String, Set<String>> mimeTypes;
      static Path getResourcesPath ();

      Path path;
      Mutex mutex;
    #if SSC_PLATFORM_APPLE
      NSURL* url = nullptr;
    #endif

      FileResource (const String& resourcePath);
      ~FileResource ();
      FileResource (const FileResource&);
      FileResource (FileResource&&);
      FileResource& operator= (const FileResource&);
      FileResource& operator= (FileResource&&);

      bool startAccessing ();
      bool stopAccessing ();
      bool exists () const noexcept;
      bool hasAccess () const noexcept;
      const String mimeType () const noexcept;
      size_t size (bool cached = false) noexcept;
      const char* read (bool cached = false);
      const String string (bool cached = false);
  };
}

#endif
