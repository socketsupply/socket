#include "core.hh"
#include "../process/process.hh"
#include "platform.hh"

#ifdef __APPLE__
  #include <TargetConditionals.h>

  #if TARGET_OS_MAC && !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    #include <Foundation/Foundation.h>
  #endif
#endif

namespace SSC {
  extern const RuntimePlatform platform = {
  #if defined(__x86_64__) || defined(_M_X64)
    .arch = "x86_64",
  #elif defined(__aarch64__) || defined(_M_ARM64)
    .arch = "arm64",
  #elif defined(__i386__) && !defined(__ANDROID__)
  #  error Socket is not supported on i386.
  #else
    .arch = "unknown",
  #endif

  // Windows
  #if defined(_WIN32)
    .os = "win32",
    .win = true,
  #endif

  // macOS & iOS
  #if defined(__APPLE__)
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    .os = "ios",
    .ios = true,
    #else
    .os = "mac",
    .mac = true,
    #endif
    #if defined(__unix__) || defined(unix) || defined(__unix)
    .unix = true
    #else
    .unix = false
    #endif
  #endif

  // Android & Linux
  #if defined(__linux__)
    #undef linux
    #ifdef __ANDROID__
    .os = "android",
    .android = true,
    .linux = true,
    #else
    .os = "linux",
    .linux = true,
    #endif

    #if defined(__unix__) || defined(unix) || defined(__unix)
    .unix = true
    #else
    .unix = false
    #endif
  #endif

  // FreeBSD
  #if defined(__FreeBSD__)
    .os = "freebsd",
    #if defined(__unix__) || defined(unix) || defined(__unix)
    .unix = true
    #else
    .unix = false
    #endif
  #endif

  // OpenBSD (possibly)
  #if !defined(__APPLE__) && defined(BSD) && (defined(__unix__) || defined(unix) || defined(__unix))
    .os = "openbsd",
    .unix = true
  #endif
  };

  void Core::Platform::event (
    const String seq,
    const String event,
    const String data,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      // init page
      if (event == "domcontentloaded") {
        Lock lock(this->core->fs.mutex);

        this->core->domReady = true;

        for (auto const &tuple : this->core->fs.descriptors) {
          auto desc = tuple.second;
          if (desc != nullptr) {
            desc->stale = true;
          } else {
            this->core->fs.descriptors.erase(tuple.first);
          }
        }

        #if !defined(__ANDROID__)
        for (auto const &tuple : this->core->fs.watchers) {
          auto watcher = tuple.second;
          if (watcher != nullptr) {
            watcher->stop();
            delete watcher;
          }
        }

        this->core->fs.watchers.clear();
        #endif
      }

      auto json = JSON::Object::Entries {
        {"source", "platform.event"},
        {"data", JSON::Object::Entries{}}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::Platform::notify (const String seq, const String title, const String body, Module::Callback cb) {
    #if defined(__APPLE__)
      auto center = [UNUserNotificationCenter currentNotificationCenter];
      auto content = [[UNMutableNotificationContent alloc] init];
      content.body = [NSString stringWithUTF8String: body.c_str()];
      content.title = [NSString stringWithUTF8String: title.c_str()];
      content.sound = [UNNotificationSound defaultSound];

      auto trigger = [UNTimeIntervalNotificationTrigger
        triggerWithTimeInterval: 1.0f
                        repeats: NO
      ];

      auto request = [UNNotificationRequest
        requestWithIdentifier: @"LocalNotification"
                      content: content
                      trigger: trigger
      ];

      auto options = (
        UNAuthorizationOptionAlert |
        UNAuthorizationOptionBadge |
        UNAuthorizationOptionSound
      );

      [center requestAuthorizationWithOptions: options
                            completionHandler: ^(BOOL granted, NSError* error)
      {
        #if !__has_feature(objc_arc)
        [content release];
        [trigger release];
        #endif

        if (granted) {
          auto json = JSON::Object::Entries {
            {"source", "platform.notify"},
            {"data", JSON::Object::Entries {}}
          };

          cb(seq, json, Post{});
        } else if (error) {
          [center addNotificationRequest: request
                   withCompletionHandler: ^(NSError* error)
          {
            auto json = JSON::Object {};

            #if !__has_feature(objc_arc)
            [request release];
            #endif

            if (error) {
              json = JSON::Object::Entries {
                {"source", "platform.notify"},
                {"err", JSON::Object::Entries {
                  {"message", [error.debugDescription UTF8String]}
                }}
              };

              debug("Unable to create notification: %@", error.debugDescription);
            } else {
              json = JSON::Object::Entries {
                {"source", "platform.notify"},
                {"data", JSON::Object::Entries {}}
              };
            }

           cb(seq, json, Post{});
          }];
        } else {
          auto json = JSON::Object::Entries {
            {"source", "platform.notify"},
            {"err", JSON::Object::Entries {
              {"message", "Failed to create notification"}
            }}
          };

          cb(seq, json, Post{});
        }

        if (!error || granted) {
          #if !__has_feature(objc_arc)
          [request release];
          #endif
        }
      }];
    #endif
  }

  void Core::Platform::revealFile (const String seq, const String value, Module::Callback cb) {
    String errorMessage = "Failed to open external file";
    String pathToFile = decodeURIComponent(value);
    bool success = false;
    auto json = JSON::Object(JSON::Object::Entries {
      {"source", "platform.revealFile"}
    });

    #if SSC_PLATFORM_MACOS
      success = [NSWorkspace.sharedWorkspace
                      selectFile: nil
        inFileViewerRootedAtPath: @(pathToFile.c_str())
      ];
    #elif SSC_PLATFORM_LINUX
      const auto result = exec("xdg-open " + pathToFile);
      success = result.exitCode == 0;
      errorMessage = result.output;
    #elif SSC_PLATFORM_WINDOWS
      const auto result = exec("explorer.exe \"" + pathToFile + "\"");
      success = result.exitCode == 0;
      errorMessage = result.output;
    #endif

    if (!success) {
      json["err"] = JSON::Object::Entries {
        {"message", errorMessage}
      };
    } else {
      json["data"] = JSON::Object {};
    }

    cb(seq, json, Post{});
  }

  void Core::Platform::openExternal (const String seq, const String value, Module::Callback cb) {
    #if SSC_PLATFORM_APPLE
      const auto url = [NSURL URLWithString: @(value.c_str())];

      #if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
        auto app = [UIApplication sharedApplication];
        [app openURL: url options: @{} completionHandler: ^(BOOL success) {
          auto json = JSON::Object {};

          if (!success) {
            json = JSON::Object::Entries {
              {"source", "platform.openExternal"},
              {"err", JSON::Object::Entries {
                {"message", "Failed to open external URL"}
              }}
            };
          } else {
            json = JSON::Object::Entries {
              {"source", "platform.openExternal"},
              {"data", JSON::Object::Entries {{"url", value}}}
            };
          }

          cb(seq, json, Post{});
        }];
      #else
        auto workspace = [NSWorkspace sharedWorkspace];
        auto configuration = [NSWorkspaceOpenConfiguration configuration];
        [workspace openURL: url
             configuration: configuration
         completionHandler: ^(NSRunningApplication *app, NSError *error)
        {
           auto json = JSON::Object {};
           if (error) {
             if (error.debugDescription.UTF8String) {
               debug("%s", error.debugDescription.UTF8String);
             }

             json = JSON::Object::Entries {
               {"source", "platform.openExternal"},
               {"err", JSON::Object::Entries {
                 {"message", error.localizedDescription.UTF8String}
               }}
             };
           } else {
            json = JSON::Object::Entries {
              {"source", "platform.openExternal"},
              {"data", JSON::Object::Entries {{ "url", value}}}
            };
           }

          cb(seq, json, Post{});
        }];
      #endif
    #elif SSC_PLATFORM_LINUX
      auto list = gtk_window_list_toplevels();
      auto json = JSON::Object {};

      // initial state is a failure
      json = JSON::Object::Entries {
        {"source", "platform.openExternal"},
        {"err", JSON::Object::Entries {
          {"message", "Failed to open external URL"}
        }}
      };

      if (list != nullptr) {
        for (auto entry = list; entry != nullptr; entry = entry->next) {
          auto window = GTK_WINDOW(entry->data);

          if (window != nullptr && gtk_window_is_active(window)) {
            auto err = (GError*) nullptr;
            auto uri = value.c_str();
            auto ts = GDK_CURRENT_TIME;

            /**
             * GTK may print a error in the terminal that looks like:
             *
             *   libva error: vaGetDriverNameByIndex() failed with unknown libva error, driver_name = (null)
             *
             * It doesn't prevent the URI from being opened.
             * See https://github.com/intel/media-driver/issues/1349 for more info
             */
            auto success = gtk_show_uri_on_window(window, uri, ts, &err);

            if (success) {
              json = JSON::Object::Entries {
                {"source", "platform.openExternal"},
                {"data", JSON::Object::Entries {}}
              };
            } else if (err != nullptr) {
              json = JSON::Object::Entries {
                {"source", "platform.openExternal"},
                {"err", JSON::Object::Entries {
                  {"message", err->message}
                }}
              };
            }

            break;
          }
        }

        g_list_free(list);
      }

      cb(seq, json, Post{});
    #elif SSC_PLATFORM_WINDOWS
      auto uri = value.c_str();
      ShellExecute(nullptr, "Open", uri, nullptr, nullptr, SW_SHOWNORMAL);
      // TODO how to detect success here. do we care?
      cb(seq, JSON::Object{}, Post{});
    #else
      const auto json = JSON::Object::Entries {
        {"source", "platform.openExternal"},
        {"err", JSON::Object::Entries {
          {"type", "NotSupportedError"},
          {"message", "Operation not supported"}
        }}
      };
      cb(seq, json, Post{});
    #endif
  }
}
