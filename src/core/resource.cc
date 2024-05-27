#include "resource.hh"
#include "debug.hh"
#include "core.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../platform/android.hh"
#include <fstream>
#endif

namespace SSC {
  static std::map<String, FileResource::Cache> caches;
  static Mutex mutex;

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  Android::AssetManager* sharedAndroidAssetManager = nullptr;
#endif

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

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  static Path getRelativeAndroidAssetManagerPath (const Path& resourcePath) {
    auto resourcesPath = FileResource::getResourcesPath();
    auto assetPath = replace(resourcePath.string(), resourcesPath.string(), "");
    if (assetPath.starts_with("/")) {
      assetPath = assetPath.substr(1);
    }

    return Path(assetPath);
  }
#endif

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  void FileResource::setSharedAndroidAssetManager (Android::AssetManager* assetManager) {
    Lock lock(mutex);
    sharedAndroidAssetManager = assetManager;
  }

  Android::AssetManager* FileResource::getSharedAndroidAssetManager () {
    return sharedAndroidAssetManager;
  }
#endif

  bool FileResource::isFile (const String& resourcePath) {
    return FileResource::isFile(Path(resourcePath));
  }

  bool FileResource::isFile (const Path& resourcePath) {
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    if (sharedAndroidAssetManager) {
      const auto assetPath = getRelativeAndroidAssetManagerPath(resourcePath);
      const auto asset = AAssetManager_open(
        sharedAndroidAssetManager,
        assetPath.c_str(),
        AASSET_MODE_BUFFER
      );

      if (asset) {
        AAsset_close(asset);
        return true;
      }
    }
  #elif SOCKET_RUNTIME_PLATFORM_APPLE
    static auto fileManager = [[NSFileManager alloc] init];
    bool isDirectory = false;
    const auto fileExistsAtPath = [fileManager
      fileExistsAtPath: @(resourcePath.c_str())
           isDirectory: &isDirectory
    ];

    return fileExistsAtPath && !isDirectory;
  #endif

    return fs::is_regular_file(resourcePath);
  }

  bool FileResource::isDirectory (const String& resourcePath) {
    return FileResource::isDirectory(Path(resourcePath));
  }

  bool FileResource::isDirectory (const Path& resourcePath) {
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    if (sharedAndroidAssetManager) {
      const auto assetPath = getRelativeAndroidAssetManagerPath(resourcePath);
      const auto assetDir = AAssetManager_openDir(
        sharedAndroidAssetManager,
        assetPath.c_str()
      );

      if (assetDir) {
        AAssetDir_close(assetDir);
        return true;
      }
    }
  #elif SOCKET_RUNTIME_PLATFORM_APPLE
    static auto fileManager = [[NSFileManager alloc] init];
    bool isDirectory = false;
    const auto fileExistsAtPath = [fileManager
      fileExistsAtPath: @(resourcePath.string().c_str())
           isDirectory: &isDirectory
    ];

    return fileExistsAtPath && isDirectory;
  #endif

    return fs::is_directory(resourcePath);
  }

  Path FileResource::getResourcesPath () {
    static String value;

    if (value.size() > 0) {
      return Path(value);
    }

  #if SOCKET_RUNTIME_PLATFORM_MACOS
    static const auto resourcePath = NSBundle.mainBundle.resourcePath;
    value = resourcePath.UTF8String;
  #elif SOCKET_RUNTIME_PLATFORM_IOS || SOCKET_RUNTIME_PLATFORM_IOS_SIMULATOR
    static const auto resourcePath = NSBundle.mainBundle.resourcePath;
    value = [resourcePath stringByAppendingPathComponent: @"ui"].UTF8String;
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    static const auto self = fs::canonical("/proc/self/exe");
    value = self.parent_path().string();
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
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
    #if !PLATFORM_WINDOWS
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
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
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

  FileResource::FileResource (
    const Path& resourcePath,
    const Options& options
  ) : options(options) {
    this->path = fs::absolute(resourcePath);
    this->startAccessing();
  }

  FileResource::FileResource (const String& resourcePath, const Options& options)
    : FileResource(Path(resourcePath), options)
  {}

  FileResource::~FileResource () {
    this->stopAccessing();
  }

  FileResource::FileResource (const FileResource& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
    this->startAccessing();
  }

  FileResource::FileResource (FileResource&& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
    this->accessing = resource.accessing.load();

    resource.bytes = nullptr;
    resource.cache.size = 0;
    resource.cache.bytes = nullptr;

    this->startAccessing();
  }

  FileResource& FileResource::operator= (const FileResource& resource) {
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
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
    this->options = resource.options;
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

    if (caches.contains(this->path.string())) {
      this->cache = caches.at(this->path.string());
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->url == nullptr) {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
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
  #if SOCKET_RUNTIME_PLATFORM_APPLE
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

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    static auto fileManager = [[NSFileManager alloc] init];
    return [fileManager
      fileExistsAtPath: @(this->path.string().c_str())
           isDirectory: NULL
    ];
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    if (sharedAndroidAssetManager) {
      const auto assetPath = getRelativeAndroidAssetManagerPath(this->path);
      const auto asset = AAssetManager_open(
        sharedAndroidAssetManager,
        assetPath.c_str(),
        AASSET_MODE_BUFFER
      );

      if (asset) {
        AAsset_close(asset);
        return true;
      }
    }

    return fs::exists(this->path);
  #else
    return fs::exists(this->path);
  #endif
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

    #if SOCKET_RUNTIME_PLATFORM_APPLE
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
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      gboolean typeGuessResultUncertain = false;
      gchar* type = nullptr;

      type = g_content_type_guess(this->path.string().c_str(), nullptr, 0, &typeGuessResultUncertain);

      if (!type || typeGuessResultUncertain) {
        const auto bytes = this->read();
        const auto size = this->size();
        const auto nextType = g_content_type_guess(
          reinterpret_cast<const gchar*>(this->path.string().c_str()),
          reinterpret_cast<const guchar*>(bytes),
          (gsize) size,
          &typeGuessResultUncertain
        );

        if (nextType) {
          if (type) {
            g_free(type);
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
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
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
    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
      if (extension.size() > 1) {
        return Android::MimeTypeMap::sharedMimeTypeMap()->getMimeTypeFromExtension(
          extension.starts_with(".") ? extension.substr(1) : extension
        );
      }
    #endif

    return "";
  }

  size_t FileResource::size () const noexcept {
    return this->cache.size;
  }

  size_t FileResource::size (bool cached) noexcept {
    if (cached && this->cache.size > 0) {
      return this->cache.size;
    }

    if (!this->accessing) {
      return -1;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
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
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
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
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    bool success = false;
    if (sharedAndroidAssetManager) {
      const auto assetPath = getRelativeAndroidAssetManagerPath(this->path);
      const auto asset = AAssetManager_open(
        sharedAndroidAssetManager,
        assetPath.c_str(),
        AASSET_MODE_BUFFER
      );

      if (asset) {
        this->cache.size = AAsset_getLength(asset);
        AAsset_close(asset);
      }
    }

    if (!success) {
      if (fs::exists(this->path)) {
        this->cache.size = fs::file_size(this->path);
      }
    }
  #else
    if (fs::exists(this->path)) {
      this->cache.size = fs::file_size(this->path);
    }
  #endif

    return this->cache.size;
  }

  const char* FileResource::read () const {
    return this->cache.bytes.get();
  }

  // caller takes ownership of returned pointer
  const char* FileResource::read (bool cached) {
    if (!this->accessing || !this->exists()) {
      return nullptr;
    }

    if (cached && this->cache.bytes != nullptr) {
      return this->cache.bytes.get();
    }

    if (this->bytes != nullptr) {
      this->bytes = nullptr;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->url == nullptr) {
      return nullptr;
    }

    const auto data = [NSData dataWithContentsOfURL: this->url];
    if (data.length > 0) {
      this->bytes.reset(new char[data.length]{0});
      memcpy(this->bytes.get(), data.bytes, data.length);
    }
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    GError* error = nullptr;
    char* contents = nullptr;
    gsize size = 0;
    if (g_file_get_contents(this->path.string().c_str(), &contents, &size, &error)) {
      if (size > 0) {
        this->bytes.reset(new char[size]{0});
        memcpy(this->bytes.get(), contents, size);
      }
    }

    if (contents) {
      g_free(contents);
    }
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
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
        this->bytes.reset(bytes);
      } else {
        delete [] bytes;
      }

      CloseHandle(handle);
    }
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    bool success = false;
    if (sharedAndroidAssetManager) {
      const auto assetPath = getRelativeAndroidAssetManagerPath(this->path);
      const auto asset = AAssetManager_open(
        sharedAndroidAssetManager,
        assetPath.c_str(),
        AASSET_MODE_BUFFER
      );

      if (asset) {
        auto size = AAsset_getLength(asset);
        if (size) {
          const auto buffer = AAsset_getBuffer(asset);
          if (buffer) {
            auto bytes = new char[size]{0};
            memcpy(bytes, buffer, size);
            this->bytes.reset(bytes);
            this->cache.size = size;
            success = true;
          }
        }

        AAsset_close(asset);
      }
    }

    if (!success) {
      auto stream = std::ifstream(this->path);
      auto buffer = std::istreambuf_iterator<char>(stream);
      auto size = fs::file_size(this->path);
      auto end = std::istreambuf_iterator<char>();

      auto bytes = new char[size]{0};
      String content;

      content.assign(buffer, end);
      memcpy(bytes, content.data(), size);

      this->bytes.reset(bytes);
      this->cache.size = size;
    }
  #endif

    this->cache.bytes = this->bytes;
    if (this->options.cache) {
      caches.insert_or_assign(this->path.string(), this->cache);
    }
    return this->cache.bytes.get();
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

  FileResource::ReadStream FileResource::stream (const ReadStream::Options& options) {
    return ReadStream(ReadStream::Options(this->path, options.highWaterMark, this->size()));
  }

  FileResource::ReadStream::ReadStream (const Options& options)
    : options(options)
  {}

  FileResource::ReadStream::~ReadStream () {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    if (this->file != nullptr) {
      g_object_unref(this->file);
      this->file = nullptr;
    }

    if (this->stream != nullptr) {
      g_input_stream_close(
        reinterpret_cast<GInputStream*>(this->stream),
        nullptr,
        nullptr
      );
      g_object_unref(this->stream);
      this->stream = nullptr;
    }
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  #endif
  }

  FileResource::ReadStream::ReadStream (
    const ReadStream& stream
  ) : options(stream.options),
      offset(stream.offset.load()),
      ended(stream.ended.load())
  {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->file = stream.file;
    this->stream = stream.stream;

    if (this->file) {
      g_object_ref(this->file);
    }

    if (this->stream) {
      g_object_ref(this->stream);
    }
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  #endif
  }

  FileResource::ReadStream::ReadStream (ReadStream&& stream)
    : options(stream.options),
      offset(stream.offset.load()),
      ended(stream.ended.load())
  {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->file = stream.file;
    this->stream = stream.stream;
    if (stream.file) {
      stream.file = nullptr;
    }

    if (stream.stream) {
      stream.stream = nullptr;
    }
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  #endif
  }

  FileResource::ReadStream& FileResource::ReadStream::operator = (
    const ReadStream& stream
  ) {
    this->options.highWaterMark = stream.options.highWaterMark;
    this->options.resourcePath = stream.options.resourcePath;
    this->options.size = stream.options.size;
    this->offset = stream.offset.load();
    this->ended = stream.ended.load();

  #if SOCKET_RUNTIME_PLATFORM_APPLE
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->file = stream.file;
    this->stream = stream.stream;

    if (this->file) {
      g_object_ref(this->file);
    }

    if (this->stream) {
      g_object_ref(this->stream);
    }
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  #endif
    return *this;
  }

  FileResource::ReadStream& FileResource::ReadStream::operator = (
    ReadStream&& stream
  ) {
    this->options.highWaterMark = stream.options.highWaterMark;
    this->options.resourcePath = stream.options.resourcePath;
    this->options.size = stream.options.size;
    this->offset = stream.offset.load();
    this->ended = stream.ended.load();

  #if SOCKET_RUNTIME_PLATFORM_APPLE
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->file = stream.file;
    this->stream = stream.stream;
    if (stream.file) {
      stream.file = nullptr;
    }

    if (stream.stream) {
      stream.stream = nullptr;
    }
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  #endif
    return *this;
  }

  const FileResource::ReadStream::Buffer FileResource::ReadStream::read (off_t offset, size_t highWaterMark) {
    if (offset == -1) {
      offset = this->offset;
    }

    if (highWaterMark == -1) {
      highWaterMark = this->options.highWaterMark;
    }

    const auto remaining = this->remaining(offset);
    const auto size = highWaterMark > remaining ? remaining : highWaterMark;
    auto buffer = Buffer(size);

    if (buffer.size > 0 && buffer.bytes != nullptr) {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      if (this->data == nullptr) {
        auto url = [NSURL fileURLWithPath: @(this->options.resourcePath.string().c_str())];
        this->data = [NSData dataWithContentsOfURL: url];
        @try {
          [this->data
            getBytes: buffer.bytes.get()
               range: NSMakeRange(offset, size)
          ];
        } @catch (NSException* error) {
          this->error = [NSError
              errorWithDomain: error.name
                         code: 0
                     userInfo: @{
                        NSUnderlyingErrorKey: error,
                  NSDebugDescriptionErrorKey: error.userInfo ?: @{},
            NSLocalizedFailureReasonErrorKey: (error.reason ?: @"???")
          }];
        }
      }
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      if (this->file == nullptr) {
        this->file = g_file_new_for_path(this->options.resourcePath.c_str());
      }

      if (this->stream == nullptr) {
        this->stream = g_file_read(this->file, nullptr, nullptr);
      }

      if (size == 0 || highWaterMark == 0) {
        return buffer;
      }

      if (offset > this->offset) {
        g_input_stream_skip(
          reinterpret_cast<GInputStream*>(this->stream),
          offset - this->offset,
          nullptr,
          nullptr
        );

        this->offset = offset;
      }

      buffer.size = g_input_stream_read(
        reinterpret_cast<GInputStream*>(this->stream),
        buffer.bytes.get(),
        size,
        nullptr,
        &this->error
      );

    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    #endif
    }

    if (this->error) {
      buffer.size = 0;
      debug(
        "FileResource::ReadStream: read error: %s",
    #if SOCKET_RUNTIME_PLATFORM_APPLE
        error.localizedDescription.UTF8String
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
        error->message
    #else
        "An unknown error occurred"
    #endif
      );
    }

    if (buffer.size <= 0) {
      buffer.bytes = nullptr;
      buffer.size = 0;
      this->ended = true;
    } else {
      this->offset += buffer.size;
      this->ended = this->offset >= this->options.size;
    }

    return buffer;
  }

  size_t FileResource::ReadStream::remaining (off_t offset) const {
    const auto size = this->options.size;
    if (offset > -1) {
      return size - offset;
    }

    return size - this->offset;
  }

  FileResource::ReadStream::Buffer::Buffer (size_t size)
    : bytes(std::make_shared<char[]>(size)),
      size(size)
  {
    memset(this->bytes.get(), 0, size);
  }

  FileResource::ReadStream::Buffer::Buffer (const Options& options)
    : bytes(std::make_shared<char[]>(options.highWaterMark)),
      size(0)
  {
    memset(this->bytes.get(), 0, options.highWaterMark);
  }

  FileResource::ReadStream::Buffer::Buffer (const Buffer& buffer) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
  }

  FileResource::ReadStream::Buffer::Buffer (Buffer&& buffer) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
    buffer.size = 0;
    buffer.bytes = nullptr;
  }

  FileResource::ReadStream::Buffer& FileResource::ReadStream::Buffer::operator = (
    const Buffer& buffer
  ) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
    return *this;
  }

  FileResource::ReadStream::Buffer& FileResource::ReadStream::Buffer::operator = (
    Buffer&& buffer
  ) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
    buffer.size = 0;
    buffer.bytes = nullptr;
    return *this;
  }

  bool FileResource::ReadStream::Buffer::isEmpty () const {
    return this->size == 0 || this->bytes == nullptr;
  }
}
