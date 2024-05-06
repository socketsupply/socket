#include "resource.hh"
#include "debug.hh"
#include "core.hh"

namespace SSC {
  std::map<String, Set<String>> FileResource::mimeTypes = {
    {"application/font-woff", { ".woff" }},
    {"application/font-woff2", { ".woff2" }},
    {"application/x-font-opentype", { ".otf" }},
    {"application/x-font-ttf", { ".ttf" }},
    {"application/json", { ".json" }},
    {"application/ld+json", { ".jsonld" }},
    {"application/typescript", { ".ts", ".tsx" }},
    {"application/wasm", { ".wasm" }},
    {"audio/opus", { ".opux" }},
    {"audio/ogg", { ".oga" }},
    {"audio/mp3", { ".mp3" }},
    {"image/jpeg", { ".jpg", ".jpeg" }},
    {"image/png", { ".png" }},
    {"image/svg+xml", { ".svg" }},
    {"image/vnd.microsoft.icon", { ".ico" }},
    {"text/css", { ".css" }},
    {"text/html", { ".html" }},
    {"text/javascript", { ".js", ".cjs", ".mjs" }},
    {"text/plain", { ".txt", ".text" }},
    {"video/mp4", { ".mp4" }},
    {"video/mpeg", { ".mpeg" }},
    {"video/ogg", { ".ogv" }}
  };

  Path FileResource::getResourcesPath () {
    static String value;

    if (value.size() > 0) {
      return Path(value);
    }

  #if SSC_PLATFORM_MACOS
    static const auto resourcePath = NSBundle.mainBundle.resourcePath;
    value = resourcePath.UTF8String;
  #elif SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
    static const auto resourcePath = NSBundle.mainBundle.resourcePath;
    value = [resourcePath stringByAppendingPathComponent: @"ui"].UTF8String;
  #elif SSC_PLATFORM_LINUX
    static const auto self = fs::canonical("/proc/self/exe");
    value = self.parent_path().string();
  #elif SSC_PLATFORM_WINDOWS
    static wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    const auto self = Path(filename).remove_filename();
    value = path.string();
    size_t offset = 0;
    // escape
    while ((offset = value.find('\\', offset)) != Sstring::npos) {
      value.replace(offset, 1, "\\\\");
      offset += 2;
    }
  #else
    value = getcwd_state_value();
  #endif

    if (value.size() > 0) {
    #if !SSC_PLATFORM_WINDOWS
      std::replace(
        value.begin(),
        value.end(),
        '\\',
        '/'
      );
    #endif
    }

    if (value.ends_with("/")) {
      value = value.substr(0, value.size() - 1);
    }

    return Path(value);
  }

  Path FileResource::getResourcePath (const Path& resourcePath) {
    return FileResource::getResourcePath(resourcePath.string());
  }

  Path FileResource::getResourcePath (const String& resourcePath) {
    const auto resourcesPath = FileResource::getResourcesPath();
  #if SSC_PLATFORM_WINDOWS
    if (resourcePath.starts_with("\\")) {
      return Path(resourcesPath.string() + resourcePath);
    }

    return Path(resourcesPath.string() + "\\" + resourcePath);
  #else
    if (resourcePath.starts_with("/")) {
      return Path(resourcesPath.string() + resourcePath);
    }

    return Path(resourcesPath.string() + "/" + resourcePath);
  #endif
  }

  FileResource::FileResource (const String& resourcePath) {
    this->path = fs::absolute(Path(resourcePath));

    this->startAccessing();
  }

  FileResource::~FileResource () {
    this->stopAccessing();
  }

  FileResource::FileResource (const FileResource& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->accessing = resource.accessing.load();

    if (this->accessing) {
      this->startAccessing();
    }
  }

  FileResource::FileResource (FileResource&& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->accessing = resource.accessing.load();

    resource.bytes = nullptr;
    resource.cache.size = 0;
    resource.cache.bytes = nullptr;

    if (this->accessing) {
      this->startAccessing();
    }
  }

  FileResource& FileResource::operator= (const FileResource& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->accessing = resource.accessing.load();

    if (this->accessing) {
      this->startAccessing();
    }

    return *this;
  }

  FileResource& FileResource::operator= (FileResource&& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->accessing = resource.accessing.load();

    resource.bytes = nullptr;
    resource.cache.size = 0;
    resource.cache.bytes = nullptr;

    if (this->accessing) {
      this->startAccessing();
    }

    return *this;
  }

  bool FileResource::startAccessing () {
    static const auto resourcesPath = FileResource::getResourcesPath();

    if (this->accessing) {
      return false;
    }

  #if SSC_PLATFORM_APPLE
    if (this->url == nullptr) {
    #if SSC_PLATFORM_APPLE
      this->url = [NSURL fileURLWithPath: @(this->path.string().c_str())];
    #endif
    }

    if (!this->path.string().starts_with(resourcesPath.string())) {
      if (![this->url startAccessingSecurityScopedResource]) {
        return false;
      }
    }
  #endif

    this->accessing = true;
    return true;
  }

  bool FileResource::stopAccessing () {
    if (!this->accessing) {
      return false;
    }
  #if SSC_PLATFORM_APPLE
    if (this->url != nullptr) {
      [this->url stopAccessingSecurityScopedResource];
    }
  #endif
    this->accessing = false;
    return true;
  }

  bool FileResource::exists () const noexcept {
    if (!this->accessing) {
      return false;
    }

  #if SSC_PLATFORM_APPLE
    static auto fileManager = [[NSFileManager alloc] init];
    return [fileManager
      fileExistsAtPath: @(this->path.string().c_str())
           isDirectory: NULL
    ];
  #endif

    return false;
  }

  bool FileResource::hasAccess () const noexcept {
    return this->accessing;
  }

  const String FileResource::mimeType () const noexcept {
    if (!this->accessing) {
      return "";
    }

    const auto extension = this->path.extension().string();
    if (extension.size() > 0) {
      // try in memory simle mime db
      for (const auto& entry : FileResource::mimeTypes) {
        const auto& mimeType = entry.first;
        const auto& extensions = entry.second;
        if (extensions.contains(extension)) {
          return mimeType;
        }
      }
    }

    #if SSC_PLATFORM_APPLE
    if (extension.size() > 0) {
      @try {
        const auto types = [UTType
              typesWithTag: @(extension.c_str())
                  tagClass: UTTagClassFilenameExtension
          conformingToType: nullptr
        ];

        if (types.count > 0 && types.firstObject.preferredMIMEType != nullptr) {
          return types.firstObject.preferredMIMEType.UTF8String;
        }
      } @catch (::id) {}
    }
    #elif SSC_PLATFORM_LINUX
      gboolean typeGuessResultUncertain = false;
      gchar* type = nullptr;

      type = g_content_type_guess(path.c_str(), nullptr, 0, &typeGuessResultUncertain);

      if (!type || typeGuessResultUncertain) {
        const auto bytes = this->read(true);
        const auto size = this->size(true);
        const auto nextType = g_content_type_guess(
          path.c_str(),
          data,
          bytes,
          &typeGuessResultUncertain
        );

        if (nextType) {
          if (type) {
            g_free(type)
          }

          type = nextType;
        }
      }

      if (type == nullptr) {
        return "";
      }

      const auto mimeType = String(type);

      if (type) {
        g_free(type);
      }

      return mimeType;
    #elif SSC_PLATFORM_WINDOWS
      LPWSTR mimeData;
      const auto bytes = this->read(true)
      const auto size = this->size(true);
      const auto result = FindMimeFromData(
        nullptr, // (LPBC) ignored (unsused)
        convertStringToWString(path).c_str(), // filename
        reinterpret_cast<const void*>(bytes), // detected buffer data
        (DWORD) size, // detected buffer size
        nullptr, // mime suggestion
        0, // flags (unsused)
        &mimeData, // output
        0 // reserved (unsused)
      );

      if (result == S_OK) {
        return convertWStringToString(WString(mimeData));
      }
    #endif

    return "";
  }

  size_t FileResource::size (bool cached) noexcept {
    if (cached && this->cache.size > 0) {
      return this->cache.size;
    }

    if (!this->accessing) {
      return -1;
    }

  #if SSC_PLATFORM_APPLE
    if (this->url != nullptr) {
      NSNumber* size = nullptr;
      NSError* error = nullptr;
      [this->url getResourceValue: &size
        forKey: NSURLFileSizeKey
         error: &error
      ];

      if (error) {
        return -error.code;
      }

      this->cache.size = size.longLongValue;
    }
  #elif SSC_PLATFORM_WINDOWS
    LARGE_INTEGER fileSize;
    auto handle = CreateFile(
      this->path.c_str(),
      GENERIC_READ, // access
      FILE_SHARE_READ, // share mode
      nullptr, // security attribues (unused)
      OPEN_EXISTING, // creation disposition
      nullptr, // flags and attribues (unused)
      nullptr // templte file (unused)
    );

    if (handle) {
      auto result = GetFileSizeEx(handle, &fileSize);
      CloseHandle(handle);
      this->cache.size = fileSize.QuadPart;
    } else {
      return -2;
    }
  #else
    this->cache.size = fs::file_size(this->path);
  #endif

    return this->cache.size;
  }

  // caller takes ownership of returned pointer
  const char* FileResource::read (bool cached) {
    if (!this->accessing || !this->exists()) {
      return nullptr;
    }

    if (cached && this->cache.bytes != nullptr) {
      return *this->cache.bytes;
    }

    if (this->bytes != nullptr) {
      this->bytes = nullptr;
    }

  #if SSC_PLATFORM_APPLE
    if (this->url == nullptr) {
      return nullptr;
    }

    const auto data = [NSData dataWithContentsOfURL: this->url];
    if (data.length > 0) {
      this->bytes = std::make_shared<char*>(new char[data.length]{0});
      memcpy(*this->bytes, data.bytes, data.length);
    }
  #elif SSC_PLATFORM_LINUX
    char* contents = nullptr;
    gsize size = 0;
    if (g_file_get_contents(this->path.string().c_str(), &contents, &size, &error)) {
      if (size > 0) {
        this->bytes = std::make_shared<char*>(new char[size]{0});
        memcpy(*this->bytes, contents, size);
      }
    }

    if (contents) {
      g_free(contents);
    }
  #elif SSC_PLATFORM_WINDOWS
    auto handle = CreateFile(
      this->path.c_str(),
      GENERIC_READ, // access
      FILE_SHARE_READ, // share mode
      nullptr, // security attribues (unused)
      OPEN_EXISTING, // creation disposition
      nullptr, // flags and attribues (unused)
      nullptr // templte file (unused)
    );

    if (handle) {
      const auto size = this->size();
      auto bytes = new char[size]{0};
      auto result = ReadFile(
        handle, // File handle
        reinterpret_cast<void*>(bytes),
        (DWORD) size, // output buffer size
        nullptr, // bytes read (unused)
        nullptr // ignored (unused)
      );

      if (result) {
        this->bytes = std::make_shared<char*>(bytes);
      } else {
        delete [] bytes;
      }

      CloseHandle(handle);
    }
  #endif

    this->cache.bytes = this->bytes;
    return *this->cache.bytes;
  }

  const String FileResource::str (bool cached) {
    if (!this->accessing || !this->exists()) {
      return "";
    }

    const auto size = this->size(cached);
    const auto bytes = this->read(cached);

    if (bytes != nullptr && size > 0) {
      return String(bytes, size);
    }

    return "";
  }
}
