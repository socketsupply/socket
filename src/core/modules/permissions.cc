#include "../core.hh"
#include "permissions.hh"

namespace SSC {
  bool CorePermissions::hasRuntimePermission (const String& permission) const {
    static const auto userConfig = getUserConfig();
    const auto key = String("permissions_allow_") + replace(permission, "-", "_");

    if (!userConfig.contains(key)) {
      return true;
    }

    return userConfig.at(key) != "false";
  }

  void CorePermissions::query (
    const String& seq,
    const String& name,
    const Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      if (!this->hasRuntimePermission(name)) {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"},
            {"reason", "Runtime permission is disabled for '" + name + "'"}
          }}
        };
        callback(seq, json, Post{});
        return;
      }

      if (name == "geolocation") {
        JSON::Object json;

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        if (this->core->geolocation.locationObserver.isAuthorized) {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
        } else if (this->core->geolocation.locationObserver.locationManager) {
          const auto authorizationStatus = (
            this->core->geolocation.locationObserver.locationManager.authorizationStatus
          );

          if (authorizationStatus == kCLAuthorizationStatusDenied) {
            json = JSON::Object::Entries {
              {"data", JSON::Object::Entries {
                {"state", "denied"}}
              }
            };
          } else {
            json = JSON::Object::Entries {
              {"data", JSON::Object::Entries {
                {"state", "prompt"}}
              }
            };
          }
        }
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = Android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasCoarseLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_COARSE_LOCATION")
        );

        // `activity.checkPermission(permission)`
        const auto hasFineLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_FINE_LOCATION")
        );

        if (!hasCoarseLocation || !hasFineLocation) {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "prompt"}}
            }
          };
        } else {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
        }
      #endif

        callback(seq, json, Post{});
      }

      if (name == "notifications") {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"}
          }}
        };

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        const auto notificationCenter = UNUserNotificationCenter.currentNotificationCenter;
        [notificationCenter getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings* settings) {
          JSON::Object json;

          if (!settings) {
            json["err"] = JSON::Object::Entries {{ "message", "Failed to reach user notification settings" }};
          } else if (settings.authorizationStatus == UNAuthorizationStatusDenied) {
            json["data"] = JSON::Object::Entries {{"state", "denied"}};
          } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
            json["data"] = JSON::Object::Entries {{"state", "prompt"}};
          } else {
            json["data"] = JSON::Object::Entries {{"state", "granted"}};
          }

          callback(seq, json, Post{});
        }];
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = Android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasPostNotifications = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.POST_NOTIFICATIONS")
        );

        // `activity.isNotificationManagerEnabled()`
        const auto isNotificationManagerEnabled = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "isNotificationManagerEnabled",
          "()Z"
        );

        if (!hasPostNotifications || !isNotificationManagerEnabled) {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "prompt"}}
            }
          };
        } else {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
        }

        callback(seq, json, Post{});
      #else
        callback(seq, json, Post{});
      #endif
      }
    });
  }

  void CorePermissions::request (
    const String& seq,
    const String& name,
    const Map& options,
    const Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      if (!this->hasRuntimePermission(name)) {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"}}
          }
        };
        callback(seq, json, Post{});
        return;
      }

      if (name == "geolocation") {
        JSON::Object json;

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        const auto performedActivation = [router->bridge->core->geolocation.locationObserver attemptActivationWithCompletion: ^(BOOL isAuthorized) {
          if (!isAuthorized) {
            auto reason = @("Location observer could not be activated");

            if (!router->bridge->core->geolocation.locationObserver.locationManager) {
              reason = @("Location observer manager is not initialized");
            } else if (!router->bridge->core->geolocation.locationObserver.locationManager.location) {
              reason = @("Location observer manager could not provide location");
            }

            debug("%s", reason.UTF8String);
          }

          if (isAuthorized) {
            json["data"] = JSON::Object::Entries {{"state", "granted"}};
          } else if (router->bridge->core->geolocation.locationObserver.locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined) {
            json["data"] = JSON::Object::Entries {{"state", "prompt"}};
          } else {
            json["data"] = JSON::Object::Entries {{"state", "denied"}};
          }

          callback(seq, json, Post{});
        }];

        if (!performedActivation) {
          auto err = JSON::Object::Entries {{ "message", "Location observer could not be activated" }};
          err["type"] = "GeolocationPositionError";
          json["err"] = err;
          return callback(seq, json, Post{});
        }
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = Android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasCoarseLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_COARSE_LOCATION")
        );

        // `activity.checkPermission(permission)`
        const auto hasFineLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_FINE_LOCATION")
        );

        if (!hasCoarseLocation || !hasFineLocation) {
          CoreGeolocation::PermissionChangeObserver observer;
          auto permissions = attachment.env->NewObjectArray(
            2,
            attachment.env->FindClass("java/lang/String"),
            0
          );

          attachment.env->SetObjectArrayElement(
            permissions,
            0,
            attachment.env->NewStringUTF("android.permission.ACCESS_COARSE_LOCATION")
          );

          attachment.env->SetObjectArrayElement(
            permissions,
            1,
            attachment.env->NewStringUTF("android.permission.ACCESS_FINE_LOCATION")
          );

          this->core->geolocation.addPermissionChangeObserver(observer, [seq, observer, callback, this](auto result) mutable {
            JSON::Object json = JSON::Object::Entries {
              {"data", result}
            };
            callback(seq, json, Post{});
            this->core->dispatchEventLoop([=] () {
              this->core->geolocation.removePermissionChangeObserver(observer);
            });
          });

          CallVoidClassMethodFromAndroidEnvironment(
            attachment.env,
            this->core->platform.activity,
            "requestPermissions",
            "([Ljava/lang/String;)V",
            permissions
          );
        } else {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
          callback(seq, json, Post{});
        }
      #endif
      }

      if (name == "notifications") {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"}
          }}
        };

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        UNAuthorizationOptions requestOptions = UNAuthorizationOptionProvisional;
        auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
        auto requestAlert = options.contains("alert") && options.at("alert") == "true";
        auto requestBadge = options.contains("badge") && options.at("badge") == "true";
        auto requestSound = options.contains("sound") && options.at("sound") == "true";

        if (requestAlert) {
          requestOptions |= UNAuthorizationOptionAlert;
        }

        if (requestBadge) {
          requestOoptions |= UNAuthorizationOptionBadge;
        }

        if (requestSound) {
          requestOptions |= UNAuthorizationOptionSound;
        }

        if (requestAlert && requestSound) {
          requestOptions |= UNAuthorizationOptionCriticalAlert;
        }

        [notificationCenter
          requestAuthorizationWithOptions: requestOptions
                        completionHandler: ^(BOOL granted, NSError *error)
        {
          [notificationCenter
            getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings)
          {
            JSON::Object json;

            if (settings.authorizationStatus == UNAuthorizationStatusDenied) {
              json["data"] = JSON::Object::Entries {{"state", "denied"}};
            } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
              if (error) {
                const auto message = String(
                  error.localizedDescription.UTF8String != nullptr
                    ? error.localizedDescription.UTF8String
                    : "An unknown error occurred"
                );

                json["err"] = JSON::Object::Entries {{"message", message}};
              } else {
                json["data"] = JSON::Object::Entries {
                  {"state", granted ? "granted" : "denied" }
                };
              }
            } else {
              json["data"] = JSON::Object::Entries {{"state", "granted"}};
            }

            callback(seq, json, Post{});
          }];
        }];
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = Android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasPostNotifications = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.POST_NOTIFICATIONS")
        );

        // `activity.isNotificationManagerEnabled()`
        const auto isNotificationManagerEnabled = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "isNotificationManagerEnabled",
          "()Z"
        );

        if (!hasPostNotifications || !isNotificationManagerEnabled) {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "prompt"}}
            }
          };
        } else {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
        }

        if (!hasPostNotifications || !isNotificationManagerEnabled) {
          CoreNotifications::PermissionChangeObserver observer;
          auto permissions = attachment.env->NewObjectArray(
            1,
            attachment.env->FindClass("java/lang/String"),
            0
          );

          attachment.env->SetObjectArrayElement(
            permissions,
            0,
            attachment.env->NewStringUTF("android.permission.POST_NOTIFICATIONS")
          );

          this->core->notifications.addPermissionChangeObserver(observer, [=](auto result) mutable {
            JSON::Object json = JSON::Object::Entries {
              {"data", result}
            };
            callback(seq, json, Post{});
            this->core->dispatchEventLoop([=]() {
              this->core->notifications.removePermissionChangeObserver(observer);
            });
          });

          CallVoidClassMethodFromAndroidEnvironment(
            attachment.env,
            this->core->platform.activity,
            "requestPermissions",
            "([Ljava/lang/String;)V",
            permissions
          );
        } else {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
          callback(seq, json, Post{});
        }
      #else
        callback(seq, json, Post{});
      #endif
      }
    });
  }
}
