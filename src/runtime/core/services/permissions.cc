#include "../../debug.hh"
#include "../../config.hh"
#include "../../string.hh"
#include "../../runtime.hh"

#include "../services.hh"
#include "permissions.hh"

using ssc::runtime::config::getUserConfig;
using ssc::runtime::string::replace;

namespace ssc::runtime::core::services {
  bool Permissions::hasRuntimePermission (const String& permission) const {
    return this->context.getRuntime()->hasPermission(permission);
  }

  void Permissions::query (
    const String& seq,
    const String& name,
    const Callback callback
  ) const {
    this->loop.dispatch([=, this]() {
      if (!this->hasRuntimePermission(name)) {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"},
            {"reason", "Runtime permission is disabled for '" + name + "'"}
          }}
        };
        callback(seq, json, QueuedResponse{});
        return;
      }

      if (name == "geolocation") {
        JSON::Object json;

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        if (this->services.geolocation.locationObserver.isAuthorized) {
          json = JSON::Object::Entries {
            {"data", JSON::Object::Entries {
              {"state", "granted"}}
            }
          };
        } else if (this->services.geolocation.locationObserver.locationManager) {
          const auto authorizationStatus = (
            this->services.geolocation.locationObserver.locationManager.authorizationStatus
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
        const auto attachment = android::JNIEnvironmentAttachment(this->context.getRuntime()->android.jvm);
        // `activity.checkPermission(permission)`
        const auto hasCoarseLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_COARSE_LOCATION")
        );

        // `activity.checkPermission(permission)`
        const auto hasFineLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
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

        callback(seq, json, QueuedResponse{});
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

          callback(seq, json, QueuedResponse{});
        }];
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasQueuedResponseNotifications = CallClassMethodFromAndroidEnvironment(
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

        if (!hasQueuedResponseNotifications || !isNotificationManagerEnabled) {
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

        callback(seq, json, QueuedResponse{});
      #else
        callback(seq, json, QueuedResponse{});
      #endif
      }

      if (name == "camera")  {
        JSON::Object json;
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasCameraPermission = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.CAMERA")
        );

        if (!hasCameraPermission) {
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

        callback(seq, json, QueuedResponse{});
      #else
        json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "prompt"}}
          }
        };
        callback(seq, json, QueuedResponse{});
      #endif
      }

      if (name == "microphone")  {
        JSON::Object json;
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->core->platform.jvm);
        // `activity.checkPermission(permission)`
        const auto hasRecordAudioPermission = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->core->platform.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.RECORD_AUDIO")
        );

        if (!hasRecordAudioPermission) {
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

        callback(seq, json, QueuedResponse{});
      #else
        json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "prompt"}}
          }
        };
        callback(seq, json, QueuedResponse{});
      #endif
      }
    });
  }

  void Permissions::request (
    const String& seq,
    const String& name,
    const Map<String, String>& options,
    const Callback callback
  ) const {
    this->loop.dispatch([=, this]() {
      if (!this->hasRuntimePermission(name)) {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"}}
          }
        };
        callback(seq, json, QueuedResponse{});
        return;
      }

      if (name == "geolocation") {
        JSON::Object json;

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        const auto performedActivation = [this->core->geolocation.locationObserver attemptActivationWithCompletion: ^(BOOL isAuthorized) {
          if (!isAuthorized) {
            auto reason = @("Location observer could not be activated");

            if (!this->core->geolocation.locationObserver.locationManager) {
              reason = @("Location observer manager is not initialized");
            } else if (!this->core->geolocation.locationObserver.locationManager.location) {
              reason = @("Location observer manager could not provide location");
            }

            debug("%s", reason.UTF8String);
          }

          if (isAuthorized) {
            json["data"] = JSON::Object::Entries {{"state", "granted"}};
          } else if (this->core->geolocation.locationObserver.locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined) {
            json["data"] = JSON::Object::Entries {{"state", "prompt"}};
          } else {
            json["data"] = JSON::Object::Entries {{"state", "denied"}};
          }

          callback(seq, json, QueuedResponse{});
        }];

        if (!performedActivation) {
          auto err = JSON::Object::Entries {{ "message", "Location observer could not be activated" }};
          err["type"] = "GeolocationPositionError";
          json["err"] = err;
          return callback(seq, json, QueuedResponse{});
        }
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->context.getRuntime()->android.jvm);
        // `activity.checkPermission(permission)`
        const auto hasCoarseLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_COARSE_LOCATION")
        );

        // `activity.checkPermission(permission)`
        const auto hasFineLocation = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.ACCESS_FINE_LOCATION")
        );

        if (!hasCoarseLocation || !hasFineLocation) {
          Geolocation::PermissionChangeObserver observer;
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

          this->services.geolocation.addPermissionChangeObserver(observer, [this, observer, callback, seq](auto result) mutable {
            JSON::Object json = JSON::Object::Entries {
              {"data", result}
            };
            callback(seq, json, QueuedResponse{});
            this->loop.dispatch([this, observer] () {
              this->services.geolocation.removePermissionChangeObserver(observer);
            });
          });

          CallVoidClassMethodFromAndroidEnvironment(
            attachment.env,
            this->context.getRuntime()->android.activity,
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
          callback(seq, json, QueuedResponse{});
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
          requestOptions |= UNAuthorizationOptionBadge;
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

            callback(seq, json, QueuedResponse{});
          }];
        }];
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->context.getRuntime()->android.jvm);
        // `activity.checkPermission(permission)`
        const auto hasQueuedResponseNotifications = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.POST_NOTIFICATIONS")
        );

        // `activity.isNotificationManagerEnabled()`
        const auto isNotificationManagerEnabled = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "isNotificationManagerEnabled",
          "()Z"
        );

        if (!hasQueuedResponseNotifications || !isNotificationManagerEnabled) {
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

        if (!hasQueuedResponseNotifications || !isNotificationManagerEnabled) {
          Notifications::PermissionChangeObserver observer;
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

          this->services.notifications.addPermissionChangeObserver(observer, [this, observer, callback, seq](auto result) mutable {
            JSON::Object json = JSON::Object::Entries {
              {"data", result}
            };
            callback(seq, json, QueuedResponse{});
            this->loop.dispatch([this, observer]() {
              this->services.notifications.removePermissionChangeObserver(observer);
            });
          });

          CallVoidClassMethodFromAndroidEnvironment(
            attachment.env,
            this->context.getRuntime()->android.activity,
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
          callback(seq, json, QueuedResponse{});
        }
      #else
        callback(seq, json, QueuedResponse{});
      #endif
      }

      if (name == "camera") {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"}
          }}
        };
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->context.getRuntime()->android.jvm);
        // `activity.checkPermission(permission)`
        const auto hasCameraPermission = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.CAMERA")
        );

        if (!hasCameraPermission) {
          MediaDevices::PermissionChangeObserver observer;
          auto permissions = attachment.env->NewObjectArray(
            1,
            attachment.env->FindClass("java/lang/String"),
            0
          );

          attachment.env->SetObjectArrayElement(
            permissions,
            0,
            attachment.env->NewStringUTF("android.permission.CAMERA")
          );

          this->services.mediaDevices.addPermissionChangeObserver(observer, [this, observer, callback, seq](JSON::Object result) mutable {
            if (result.get("name").str() == "camera") {
              JSON::Object json = JSON::Object::Entries {
                {"data", result}
              };
              callback(seq, json, QueuedResponse{});
              this->loop.dispatch([this, observer]() {
                this->services.mediaDevices.removePermissionChangeObserver(observer);
              });
            }
          });

          CallVoidClassMethodFromAndroidEnvironment(
            attachment.env,
            this->context.getRuntime()->android.activity,
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
          callback(seq, json, QueuedResponse{});
        }
      #else
        callback(seq, json, QueuedResponse{});
      #endif
      }

      if (name == "microphone") {
        JSON::Object json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"state", "denied"}
          }}
        };
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        const auto attachment = android::JNIEnvironmentAttachment(this->context.getRuntime()->android.jvm);
        // `activity.checkPermission(permission)`
        const auto hasRecordAudioPermission = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Boolean,
          this->context.getRuntime()->android.activity,
          "checkPermission",
          "(Ljava/lang/String;)Z",
          attachment.env->NewStringUTF("android.permission.RECORD_AUDIO")
        );

        if (!hasRecordAudioPermission) {
          MediaDevices::PermissionChangeObserver observer;
          auto permissions = attachment.env->NewObjectArray(
            1,
            attachment.env->FindClass("java/lang/String"),
            0
          );

          attachment.env->SetObjectArrayElement(
            permissions,
            0,
            attachment.env->NewStringUTF("android.permission.RECORD_AUDIO")
          );

          this->services.mediaDevices.addPermissionChangeObserver(observer, [this, observer, callback, seq](JSON::Object result) mutable {
            if (result.get("name").str() == "microphone") {
              JSON::Object json = JSON::Object::Entries {
                {"data", result}
              };
              callback(seq, json, QueuedResponse{});
              this->loop.dispatch([this, observer]() {
                this->services.mediaDevices.removePermissionChangeObserver(observer);
              });
            }
          });

          CallVoidClassMethodFromAndroidEnvironment(
            attachment.env,
            this->context.getRuntime()->android.activity,
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
          callback(seq, json, QueuedResponse{});
        }
      #else
        callback(seq, json, QueuedResponse{});
      #endif
      }
    });
  }
}
