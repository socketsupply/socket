#include "../env.hh"
#include "../cwd.hh"
#include "../config.hh"
#include "../string.hh"
#include "../filesystem.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include <fstream>
#endif

using namespace ssc::runtime::string;
using ssc::runtime::config::getUserConfig;

namespace ssc::runtime::filesystem {
  static Map<String, Resource::Cache> caches;
  static Mutex mutex;
  static Resource::WellKnownPaths defaultWellKnownPaths;

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
  static const String escapeWindowsPath (const Path& path) {
    const auto dirname = Path(path).remove_filename();
    auto value = dirname.string();
    size_t offset = 0;
    // escape
    while ((offset = value.find('\\', offset)) != String::npos) {
      value.replace(offset, 1, "\\\\");
      offset += 2;
    }
    return value;
  }
  #endif

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  static Android::AssetManager* sharedAndroidAssetManager = nullptr;
  static Path externalAndroidStorageDirectory;
  static Path externalAndroidFilesDirectory;
  static Path externalAndroidCacheDirectory;
#endif

  Map<String, Set<String>> Resource::mimeTypes = {
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
    auto resourcesPath = Resource::getResourcesPath();
    auto assetPath = replace(resourcePath.string(), resourcesPath.string(), "");

    if (assetPath.starts_with("/")) {
      assetPath = assetPath.substr(1);
    } else if (assetPath.starts_with("./")) {
      assetPath = assetPath.substr(2);
    }

    return Path(assetPath);
  }
#endif

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  void Resource::setSharedAndroidAssetManager (Android::AssetManager* assetManager) {
    Lock lock(mutex);
    sharedAndroidAssetManager = assetManager;
  }

  Android::AssetManager* Resource::getSharedAndroidAssetManager () {
    return sharedAndroidAssetManager;
  }

  void Resource::setExternalAndroidStorageDirectory (const Path& directory) {
    externalAndroidStorageDirectory = directory;
  }

  Path Resource::getExternalAndroidStorageDirectory () {
    return externalAndroidStorageDirectory;
  }

  void Resource::setExternalAndroidFilesDirectory (const Path& directory) {
    externalAndroidFilesDirectory = directory;
  }

  Path Resource::getExternalAndroidFilesDirectory () {
    return externalAndroidFilesDirectory;
  }

  void Resource::setExternalAndroidCacheDirectory (const Path& directory) {
    externalAndroidCacheDirectory = directory;
  }

  Path Resource::getExternalAndroidCacheDirectory () {
    return externalAndroidCacheDirectory;
  }
#endif

  bool Resource::isFile (const String& resourcePath) {
    return Resource::isFile(Path(resourcePath));
  }

  bool Resource::isFile (const Path& resourcePath) {
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

  bool Resource::isDirectory (const String& resourcePath) {
    return Resource::isDirectory(Path(resourcePath));
  }

  bool Resource::isDirectory (const Path& resourcePath) {
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

  bool Resource::isMountedPath (const Path& path) {
    static auto userConfig = getUserConfig();
    static auto mounts = Resource::getMountedPaths();
    for (const auto& entry : mounts) {
      if (path.string().starts_with(entry.first)) {
        return true;
      }
    }
    return false;
  }

  const Map<String, String> Resource::getMountedPaths () {
    static const auto userConfig = getUserConfig();
    static Map<String, String> mounts = {}; // global

    if (mounts.size() > 0) {
      return mounts;
    }

    // determine HOME
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    static const auto HOME = runtime::env::get("HOMEPATH", runtime::env::get("USERPROFILE", runtime::env::get("HOME")));
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    static const auto HOME = String(NSHomeDirectory().UTF8String);
  #else
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : runtime::env::get("HOME", getcwd());
  #endif

    static const Map<String, String> mappings = {
      {"\\$HOST_HOME", HOME},
      {"~", HOME},

      {"\\$HOST_CONTAINER",
      #if SOCKET_RUNTIME_PLATFORM_IOS
        [NSSearchPathForDirectoriesInDomains(NSApplicationDirectory, NSUserDomainMask, YES) objectAtIndex: 0].UTF8String
      #elif SOCKET_RUNTIME_PLATFORM_MACOS
        // `homeDirectoryForCurrentUser` resolves to sandboxed container
        // directory when in "sandbox" mode, otherwise the user's HOME directory
        NSFileManager.defaultManager.homeDirectoryForCurrentUser.absoluteString.UTF8String
      #elif SOCKET_RUNTIME_PLATFORM_LINUX || SOCKET_RUNTIME_PLATFORM_ANDROID
        // TODO(@jwerle): figure out `$HOST_CONTAINER` for Linux/Android
        getcwd()
      #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
        // TODO(@jwerle): figure out `$HOST_CONTAINER` for Windows
        getcwd()
      #else
        getcwd()
      #endif
      },

      {"\\$HOST_PROCESS_WORKING_DIRECTORY",
      #if SOCKET_RUNTIME_PLATFORM_APPLE
        NSBundle.mainBundle.resourcePath.UTF8String
      #else
        getcwd()
      #endif
      }
    };

    static const auto wellKnownPaths = Resource::getWellKnownPaths();

    for (const auto& entry : userConfig) {
      if (entry.first.starts_with("webview_navigator_mounts_")) {
        auto key = replace(entry.first, "webview_navigator_mounts_", "");

        if (key.starts_with("android") && !platform.android) continue;
        if (key.starts_with("ios") && !platform.ios) continue;
        if (key.starts_with("linux") && !platform.linux) continue;
        if (key.starts_with("mac") && !platform.mac) continue;
        if (key.starts_with("win") && !platform.win) continue;

        key = replace(key, "android_", "");
        key = replace(key, "ios_", "");
        key = replace(key, "linux_", "");
        key = replace(key, "mac_", "");
        key = replace(key, "win_", "");

        String path = key;

        for (const auto& map : mappings) {
          path = replace(path, map.first, map.second);
        }

        const auto& value = entry.second;
        mounts.insert_or_assign(path, value);
      }
    }

     return mounts;
  }

  Path Resource::getResourcesPath () {
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
    #if SOCKET_RUNTIME_DESKTOP_EXTENSION
      value = getcwd();
    #else
      static const auto self = fs::canonical("/proc/self/exe");
      value = self.parent_path().string();
    #endif
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    static wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    value = escapeWindowsPath(Path(filename));
  #else
    value = getcwd_state_value();
  #endif

    if (value.size() > 0) {
    #if !SOCKET_RUNTIME_PLATFORM_WINDOWS
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

  Path Resource::getResourcePath (const Path& resourcePath) {
    return Resource::getResourcePath(resourcePath.string());
  }

  Path Resource::getResourcePath (const String& resourcePath) {
    const auto resourcesPath = Resource::getResourcesPath();
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

  void Resource::WellKnownPaths::setDefaults (const WellKnownPaths& paths) {
    defaultWellKnownPaths = paths;
  }

  const Resource::WellKnownPaths& Resource::getWellKnownPaths () {
    static const auto paths = WellKnownPaths {};
    return paths;
  }

  Resource::WellKnownPaths::WellKnownPaths () {
    static auto userConfig = getUserConfig();
    static auto bundleIdentifier = userConfig["meta_bundle_identifier"];

    // initialize default values
    this->resources = defaultWellKnownPaths.resources;
    this->downloads = defaultWellKnownPaths.downloads;
    this->documents = defaultWellKnownPaths.documents;
    this->pictures = defaultWellKnownPaths.pictures;
    this->desktop = defaultWellKnownPaths.desktop;
    this->videos = defaultWellKnownPaths.videos;
    this->music = defaultWellKnownPaths.music;
    this->config = defaultWellKnownPaths.config;
    this->home = defaultWellKnownPaths.home;
    this->data = defaultWellKnownPaths.data;
    this->log = defaultWellKnownPaths.log;
    this->tmp = defaultWellKnownPaths.tmp;

    this->resources = Resource::getResourcesPath();
    this->tmp = fs::temp_directory_path();
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : runtime::env::get("HOME", getcwd());

    static const auto fileManager = NSFileManager.defaultManager;

  #define DIRECTORY_PATH_FROM_FILE_MANAGER(type) (                             \
    String([fileManager                                                        \
        URLForDirectory: type                                                  \
               inDomain: NSUserDomainMask                                      \
      appropriateForURL: nil                                                   \
                 create: NO                                                    \
                  error: nil                                                   \
      ].path.UTF8String)                                                       \
    )

    // overload with main bundle resources path for macos/ios
    this->downloads = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSDownloadsDirectory));
    this->documents = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSDocumentDirectory));
    this->pictures = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSPicturesDirectory));
    this->desktop = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSDesktopDirectory));
    this->videos = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSMoviesDirectory));
    this->config = Path(HOME + "/Library/Application Support/" + bundleIdentifier);
    this->media = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSSharedPublicDirectory));
    this->music = Path(DIRECTORY_PATH_FROM_FILE_MANAGER(NSMusicDirectory));
    this->home = Path(String(NSHomeDirectory().UTF8String));
    this->data = Path(HOME + "/Library/Application Support/" + bundleIdentifier);
    this->log = Path(HOME + "/Library/Logs/" + bundleIdentifier);
    this->tmp = Path(String(NSTemporaryDirectory().UTF8String));

  #undef DIRECTORY_PATH_FROM_FILE_MANAGER

  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : runtime::env::get("HOME", getcwd());

    static const auto XDG_DOCUMENTS_DIR = runtime::env::get("XDG_DOCUMENTS_DIR");
    static const auto XDG_DOWNLOAD_DIR = runtime::env::get("XDG_DOWNLOAD_DIR");
    static const auto XDG_PICTURES_DIR = runtime::env::get("XDG_PICTURES_DIR");
    static const auto XDG_DESKTOP_DIR = runtime::env::get("XDG_DESKTOP_DIR");
    static const auto XDG_VIDEOS_DIR = runtime::env::get("XDG_VIDEOS_DIR");
    static const auto XDG_MUSIC_DIR = runtime::env::get("XDG_MUSIC_DIR");
    static const auto XDG_PUBLICSHARE_DIR = runtime::env::get("XDG_PUBLICSHARE_DIR");

    static const auto XDG_CONFIG_HOME = runtime::env::get("XDG_CONFIG_HOME", HOME + "/.config");
    static const auto XDG_DATA_HOME = runtime::env::get("XDG_DATA_HOME", HOME + "/.local/share");

    if (XDG_DOCUMENTS_DIR.size() > 0) {
      this->documents = Path(XDG_DOCUMENTS_DIR);
    } else {
      this->documents = Path(HOME) / "Documents";
    }

    if (XDG_DOWNLOAD_DIR.size() > 0) {
      this->downloads = Path(XDG_DOWNLOAD_DIR);
    } else {
      this->downloads = Path(HOME) / "Downloads";
    }

    if (XDG_DESKTOP_DIR.size() > 0) {
      this->desktop = Path(XDG_DESKTOP_DIR);
    } else {
      this->desktop = Path(HOME) / "Desktop";
    }

    if (XDG_PICTURES_DIR.size() > 0) {
      this->pictures = Path(XDG_PICTURES_DIR);
    } else if (fs::exists(Path(HOME) / "Images")) {
      this->pictures = Path(HOME) / "Images";
    } else if (fs::exists(Path(HOME) / "Photos")) {
      this->pictures = Path(HOME) / "Photos";
    } else {
      this->pictures = Path(HOME) / "Pictures";
    }

    if (XDG_VIDEOS_DIR.size() > 0) {
      this->videos = Path(XDG_VIDEOS_DIR);
    } else {
      this->videos = Path(HOME) / "Videos";
    }

    if (XDG_MUSIC_DIR.size() > 0) {
      this->music = Path(XDG_MUSIC_DIR);
    } else {
      this->music = Path(HOME) / "Music";
    }

    if (XDG_PUBLICSHARE_DIR.size() > 0) {
      this->media  = Path(XDG_PUBLICSHARE_DIR);
    }

    this->config = Path(XDG_CONFIG_HOME) / bundleIdentifier;
    this->home = Path(HOME);
    this->data = Path(XDG_DATA_HOME) / bundleIdentifier;
    this->log = this->config;
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    static const auto HOME = runtime::env::get("HOMEPATH", runtime::env::get("HOME"));
    static const auto USERPROFILE = runtime::env::get("USERPROFILE", HOME);
    this->downloads = escapeWindowsPath(Path(USERPROFILE) / "Downloads");
    this->documents = escapeWindowsPath(Path(USERPROFILE) / "Documents");
    this->pictures = escapeWindowsPath(Path(USERPROFILE) / "Pictures");
    this->desktop = escapeWindowsPath(Path(USERPROFILE) / "Desktop");
    this->videos = escapeWindowsPath(Path(USERPROFILE) / "Videos");
    this->music = escapeWindowsPath(Path(USERPROFILE) / "Music");
    this->config = escapeWindowsPath(Path(runtime::env::get("APPDATA")) / bundleIdentifier);
    this->home = escapeWindowsPath(Path(USERPROFILE));
    this->data = escapeWindowsPath(Path(runtime::env::get("APPDATA")) / bundleIdentifier);
    this->log = this->config;
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    const auto storage = Resource::getExternalAndroidStorageDirectory();
    const auto cache = Resource::getExternalAndroidCacheDirectory();
    this->resources = "socket://" + bundleIdentifier;
    this->tmp = !cache.empty() ? cache : storage / "tmp";
  #endif
  }

  JSON::Object Resource::WellKnownPaths::json () const {
    return JSON::Object::Entries {
      {"resources", this->resources},
      {"downloads", this->downloads},
      {"documents", this->documents},
      {"pictures", this->pictures},
      {"desktop", this->desktop},
      {"videos", this->videos},
      {"config", this->config},
      {"media", this->media},
      {"music", this->music},
      {"home", this->home},
      {"data", this->data},
      {"log", this->log},
      {"tmp", this->tmp}
    };
  }

  const Vector<Path> Resource::WellKnownPaths::entries () const {
    auto entries = Vector<Path>();
    entries.push_back(this->resources);
    entries.push_back(this->downloads);
    entries.push_back(this->documents);
    entries.push_back(this->pictures);
    entries.push_back(this->desktop);
    entries.push_back(this->videos);
    entries.push_back(this->music);
    entries.push_back(this->media);
    entries.push_back(this->config);
    entries.push_back(this->home);
    entries.push_back(this->data);
    entries.push_back(this->log);
    entries.push_back(this->tmp);
    return entries;
  }

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  const Path Resource::getMicrosoftEdgeRuntimePath () {
    // this is something like "C:\\Users\\jwerle\\AppData\\Local\\Microsoft\\Edge SxS\\Application\\123.0.2386.0"
    static const auto EDGE_RUNTIME_DIRECTORY = trim(runtime::env::get("SOCKET_EDGE_RUNTIME_DIRECTORY"));


    return Path(
      EDGE_RUNTIME_DIRECTORY.size() > 0 && fs::exists(EDGE_RUNTIME_DIRECTORY)
        ? EDGE_RUNTIME_DIRECTORY
        : ""
    );
  }
#endif

  Resource::Resource (
    const Path& resourcePath,
    const Options& options
  ) :
    runtime::Resource("Resource", resourcePath.string())
  {
    this->url = URL(resourcePath.string());

    if (url.scheme == "socket") {
      const auto resourcesPath = Resource::getResourcesPath();
      this->path = fs::absolute(resourcesPath / url.pathname);
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      this->path = Path(url.pathname);
      this->name = getRelativeAndroidAssetManagerPath(this->path).string();
    #endif
    } else if (url.scheme == "content" || url.scheme == "android.resource") {
      this->path = resourcePath;
    } else if (url.scheme == "file") {
      this->path = Path(url.pathname);
    } else {
      this->path = fs::absolute(resourcePath);
      this->url = URL("file://" + this->path.string());
    }

    this->options = options;
    this->startAccessing();

  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    if (this->isAndroidLocalAsset()) {
      this->name = getRelativeAndroidAssetManagerPath(this->path).string();
    }
  #endif
  }

  Resource::Resource (const String& resourcePath, const Options& options)
    : Resource(Path(resourcePath), options)
  {}

  Resource::~Resource () {
    this->stopAccessing();
  }

  Resource::Resource (const Resource& resource)
    : runtime::Resource("Resource", resource.name)
  {
    this->url = resource.url;
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
    this->startAccessing();
  }

  Resource::Resource (Resource&& resource)
    : runtime::Resource("Resource", resource.name)
  {
    this->url = resource.url;
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
    this->accessing = resource.accessing.load();

    resource.url = URL {};
    resource.bytes = nullptr;
    resource.cache.size = 0;
    resource.cache.bytes = nullptr;
    resource.accessing = false;

    this->startAccessing();
  }

  Resource& Resource::operator= (const Resource& resource) {
    this->url = resource.url;
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
    this->accessing = resource.accessing.load();

    this->startAccessing();

    return *this;
  }

  Resource& Resource::operator= (Resource&& resource) {
    this->url = resource.url;
    this->path = resource.path;
    this->bytes = resource.bytes;
    this->cache = resource.cache;
    this->options = resource.options;
    this->accessing = resource.accessing.load();

    resource.url = URL {};
    resource.bytes = nullptr;
    resource.cache.size = 0;
    resource.cache.bytes = nullptr;
    resource.accessing = false;

    this->startAccessing();

    return *this;
  }

  bool Resource::startAccessing () {
    static const auto resourcesPath = Resource::getResourcesPath();

    if (this->accessing) {
      return false;
    }

    if (caches.contains(this->path.string())) {
      this->cache = caches.at(this->path.string());
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->nsURL == nullptr) {
      this->nsURL = [NSURL fileURLWithPath: @(this->path.string().c_str())];
    }
  #endif

    if (Resource::isMountedPath(this->path)) {
      this->accessing = true;
      return true;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (!this->path.string().starts_with(resourcesPath.string())) {
      if (![this->nsURL startAccessingSecurityScopedResource]) {
        this->nsURL = nullptr;
        return false;
      }
    }
  #endif

    this->accessing = true;
    return true;
  }

  bool Resource::stopAccessing () {
    if (!this->accessing) {
      return false;
    }
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->nsURL != nullptr) {
      [this->nsURL stopAccessingSecurityScopedResource];
    }
  #endif
    this->accessing = false;
    return true;
  }

  bool Resource::exists () const noexcept {
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

  int Resource::access (int mode) const noexcept {
    if (this->accessing) {
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (this->isAndroidLocalAsset() || this->isAndroidContent()) {
        if (mode == F_OK || mode == R_OK) {
          return mode;
        }
      }
    #endif

      if (mode == ::access(this->path.string().c_str(), mode)) {
        return mode;
      }
    }

    return -1; // `EPERM`
  }

  const String Resource::mimeType () const noexcept {
    const auto extension = this->path.extension().string();
    if (extension.size() > 0) {
      // try in memory simle mime db
      for (const auto& entry : Resource::mimeTypes) {
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
      const auto bytes = this->read();
      const auto size = this->size();
      const auto result = FindMimeFromData(
        nullptr, // (LPBC) ignored (unsused)
        convertStringToWString(path).c_str(), // filename
        const_cast<void*>(reinterpret_cast<const void*>(bytes)), // detected buffer data
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
        const auto value = Android::MimeTypeMap::sharedMimeTypeMap()->getMimeTypeFromExtension(
          extension.starts_with(".") ? extension.substr(1) : extension
        );

        if (value.size() > 0) {
          return value;
        }
      }

      if (this->options.contentResolver && this->url.scheme == "content") {
        return this->options.contentResolver.getContentMimeType(this->url.str());
      }
    #endif

    return "";
  }

  size_t Resource::size () const noexcept {
    return this->cache.size;
  }

  size_t Resource::size (bool cached) noexcept {
    if (cached && this->cache.size > 0) {
      return this->cache.size;
    }

    if (!this->accessing) {
      return -1;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->nsURL != nullptr) {
      NSNumber* size = nullptr;
      NSError* error = nullptr;
      [this->nsURL getResourceValue: &size
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
      convertWStringToString(this->path.string()).c_str(),
      GENERIC_READ, // access
      FILE_SHARE_READ, // share mode
      nullptr, // security attribues (unused)
      OPEN_EXISTING, // creation disposition
      0, // flags and attribues (unused)
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
      if (this->isAndroidLocalAsset()) {
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
    } else if (this->url.scheme == "content" || this->url.scheme == "android.resource") {
      if (this->options.contentResolver != nullptr) {
        off_t offset = 0;
        off_t length = 0;
        auto fileDescriptor = this->options.contentResolver->openFileDescriptor (
          this->url.str(),
          &offset,
          &length
        );

        this->options.contentResolver->closeFileDescriptor(fileDescriptor);
        return length;
      }
    }
  #else
    if (fs::exists(this->path)) {
      this->cache.size = fs::file_size(this->path);
    }
  #endif

    return this->cache.size;
  }

  const char* Resource::read () const {
    return this->cache.bytes.get();
  }

  // caller takes ownership of returned pointer
  const char* Resource::read (bool cached) {
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
    if (this->nsURL == nullptr) {
      return nullptr;
    }

    const auto data = [NSData dataWithContentsOfURL: this->nsURL];
    if (data.length > 0) {
      this->bytes.reset(new char[data.length]{0});
      memcpy(this->bytes.get(), data.bytes, data.length);
    }
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    auto span = this->tracer.span("read");
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
    span->end();
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    auto handle = CreateFile(
      convertWStringToString(this->path.string()).c_str(),
      GENERIC_READ, // access
      FILE_SHARE_READ, // share mode
      nullptr, // security attribues (unused)
      OPEN_EXISTING, // creation disposition
      0, // flags and attribues (unused)
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

  const String Resource::str (bool cached) {
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

  Resource::ReadStream Resource::stream (const ReadStream::Options& options) {
    return ReadStream(ReadStream::Options(this->path, options.highWaterMark, this->size()));
  }

  Resource::ReadStream::ReadStream (const Options& options)
    : options(options)
  {}

  Resource::ReadStream::~ReadStream () {
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

  Resource::ReadStream::ReadStream (
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

  Resource::ReadStream::ReadStream (ReadStream&& stream)
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

  Resource::ReadStream& Resource::ReadStream::operator = (
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

  Resource::ReadStream& Resource::ReadStream::operator = (
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

  const Resource::ReadStream::Buffer Resource::ReadStream::read (off_t offset, size_t highWaterMark) {
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
        "Resource::ReadStream: read error: %s",
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

  size_t Resource::ReadStream::remaining (off_t offset) const {
    const auto size = this->options.size;
    if (offset > -1) {
      return size - offset;
    }

    return size - this->offset;
  }

  Resource::ReadStream::Buffer::Buffer (size_t size)
    : bytes(std::make_shared<char[]>(size)),
      size(size)
  {
    memset(this->bytes.get(), 0, size);
  }

  Resource::ReadStream::Buffer::Buffer (const Options& options)
    : bytes(std::make_shared<char[]>(options.highWaterMark)),
      size(0)
  {
    memset(this->bytes.get(), 0, options.highWaterMark);
  }

  Resource::ReadStream::Buffer::Buffer (const Buffer& buffer) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
  }

  Resource::ReadStream::Buffer::Buffer (Buffer&& buffer) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
    buffer.size = 0;
    buffer.bytes = nullptr;
  }

  Resource::ReadStream::Buffer& Resource::ReadStream::Buffer::operator = (
    const Buffer& buffer
  ) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
    return *this;
  }

  Resource::ReadStream::Buffer& Resource::ReadStream::Buffer::operator = (
    Buffer&& buffer
  ) {
    this->size = buffer.size.load();
    this->bytes = buffer.bytes;
    buffer.size = 0;
    buffer.bytes = nullptr;
    return *this;
  }

  bool Resource::ReadStream::Buffer::isEmpty () const {
    return this->size == 0 || this->bytes == nullptr;
  }

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  bool Resource::isAndroidLocalAsset () const noexcept {
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

    return false;
  }

  bool Resource::isAndroidContent () const noexcept {
    const auto uri = this->path.string();
    if (this->options.contentResolver->isContentURI(uri)) {
      const auto pathname = this->options.contentResolver->getPathnameFromURI(uri);
      return pathname.size() > 0;
    }
    return false;
  }
#endif
}
