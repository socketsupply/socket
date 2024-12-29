#include "../../debug.hh"
#include "../../config.hh"

#include "geolocation.hh"

#if SOCKET_RUNTIME_PLATFORM_APPLE
using ssc::runtime::config::getUserConfig;
@implementation SSCLocationPositionWatcher
+ (SSCLocationPositionWatcher*) positionWatcherWithIdentifier: (NSInteger) identifier
                                                   completion: (void (^)(CLLocation*)) completion
{
  auto watcher= [SSCLocationPositionWatcher new];
  watcher.identifier = identifier;
  watcher.completion = [completion copy];
  return watcher;
}
@end

@implementation SSCLocationObserver
- (id) init {
  self = [super init];
  self.delegate = [[SSCLocationManagerDelegate alloc] initWithLocationObserver: self];
  self.isAuthorized = NO;
  self.locationWatchers = [NSMutableArray new];
  self.activationCompletions = [NSMutableArray new];
  self.locationRequestCompletions = [NSMutableArray new];

  self.locationManager = [CLLocationManager new];
  self.locationManager.delegate = self.delegate;
  self.locationManager.desiredAccuracy = CLAccuracyAuthorizationFullAccuracy;
  self.locationManager.pausesLocationUpdatesAutomatically = NO;

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
  self.locationManager.allowsBackgroundLocationUpdates = YES;
  self.locationManager.showsBackgroundLocationIndicator = YES;
#endif

  if ([CLLocationManager locationServicesEnabled]) {
    if (
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      self.locationManager.authorizationStatus == kCLAuthorizationStatusAuthorized ||
    #else
      self.locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedWhenInUse ||
    #endif
      self.locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedAlways
    ) {
      self.isAuthorized = YES;
    }
  }

  return self;
}

- (BOOL) attemptActivation {
  if ([CLLocationManager locationServicesEnabled] == NO) {
    return NO;
  }

  if (self.isAuthorized) {
    [self.locationManager requestLocation];
    return YES;
  }

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
  [self.locationManager requestWhenInUseAuthorization];
#else
  [self.locationManager requestAlwaysAuthorization];
#endif

  return YES;
}

- (BOOL) attemptActivationWithCompletion: (void (^)(BOOL)) completion {
  if (self.isAuthorized) {
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(YES);
    });
    return YES;
  }

  if ([self attemptActivation]) {
    [self.activationCompletions addObject: [completion copy]];
    return YES;
  }

  return NO;
}

- (BOOL) getCurrentPositionWithCompletion: (void (^)(NSError*, CLLocation*)) completion {
  return [self attemptActivationWithCompletion: ^(BOOL isAuthorized) {
    auto userConfig = getUserConfig();
    if (!isAuthorized) {
      auto reason = @("Location observer could not be activated");

      if (!self.locationManager) {
        reason = @("Location observer manager is not initialized");
      } else if (!self.locationManager.location) {
        reason = @("Location observer manager could not provide location");
      }

      auto error = [NSError
        errorWithDomain: @(userConfig["meta_bundle_identifier"].c_str())
        code: -1
        userInfo: @{
          NSLocalizedDescriptionKey: reason
        }
      ];

      return completion(error, nullptr);
    }

    auto location = self.locationManager.location;
    if (location.timestamp.timeIntervalSince1970 > 0) {
      completion(nullptr, self.locationManager.location);
    } else {
      [self.locationRequestCompletions addObject: [completion copy]];
    }

    [self.locationManager requestLocation];
  }];
}

- (int) watchPositionForIdentifier: (NSInteger) identifier
                        completion: (void (^)(NSError*, CLLocation*)) completion {
  SSCLocationPositionWatcher* watcher = nullptr;
  BOOL exists = NO;

  for (SSCLocationPositionWatcher* existing in self.locationWatchers) {
    if (existing.identifier == identifier) {
      watcher = existing;
      exists = YES;
      break;
    }
  }

  if (!watcher) {
    watcher = [SSCLocationPositionWatcher
      positionWatcherWithIdentifier: identifier
                         completion: ^(CLLocation* location) {
      completion(nullptr, location);
    }];
  }

  const auto performedActivation = [self attemptActivationWithCompletion: ^(BOOL isAuthorized) {
    auto userConfig = getUserConfig();
    if (!isAuthorized) {
      auto error = [NSError
        errorWithDomain: @(userConfig["meta_bundle_identifier"].c_str())
        code: -1
        userInfo: @{
          @"Error reason": @("Location observer could not be activated")
        }
      ];

      return completion(error, nullptr);
    }

    [self.locationManager startUpdatingLocation];

    if (CLLocationManager.headingAvailable) {
      [self.locationManager startUpdatingHeading];
    }

    [self.locationManager startMonitoringSignificantLocationChanges];
  }];

  if (!performedActivation) {
  #if !__has_feature(objc_arc)
    [watcher release];
    #endif
    return -1;
  }

  if (!exists) {
    [self.locationWatchers addObject: watcher];
  }

  return identifier;
}

- (BOOL) clearWatch: (NSInteger) identifier {
  for (SSCLocationPositionWatcher* watcher in self.locationWatchers) {
    if (watcher.identifier == identifier) {
      [self.locationWatchers removeObject: watcher];
    #if !__has_feature(objc_arc)
      [watcher release];
    #endif
      return YES;
    }
  }

  return NO;
}
@end

@implementation SSCLocationManagerDelegate
- (id) initWithLocationObserver: (SSCLocationObserver*) locationObserver {
  self = [super init];
  self.locationObserver = locationObserver;
  locationObserver.delegate = self;
  return self;
}

- (void) locationManager: (CLLocationManager*) locationManager
      didUpdateLocations: (NSArray<CLLocation*>*) locations {
  auto locationRequestCompletions = [NSArray arrayWithArray: self.locationObserver.locationRequestCompletions];
  for (id item in locationRequestCompletions) {
    auto completion = (void (^)(CLLocation*)) item;
    completion(locations.firstObject);
    [self.locationObserver.locationRequestCompletions removeObject: item];
  #if !__has_feature(objc_arc)
    [completion release];
  #endif
  }

  for (SSCLocationPositionWatcher* watcher in self.locationObserver.locationWatchers) {
    watcher.completion(locations.firstObject);
  }
}

- (void) locationManager: (CLLocationManager*) locationManager
        didFailWithError: (NSError*) error {
 // TODO(@jwerle): handle location manager error
  debug("locationManager:didFailWithError: %@", error);
}

- (void)            locationManager: (CLLocationManager*) locationManager
  didFinishDeferredUpdatesWithError: (NSError*) error {
 // TODO(@jwerle): handle deferred error
  debug("locationManager:didFinishDeferredUpdatesWithError: %@", error);
}

- (void) locationManagerDidPauseLocationUpdates: (CLLocationManager*) locationManager {
 // TODO(@jwerle): handle pause for updates
  debug("locationManagerDidPauseLocationUpdates");
}

- (void) locationManagerDidResumeLocationUpdates: (CLLocationManager*) locationManager {
  // TODO(@jwerle): handle resume for updates
  debug("locationManagerDidResumeLocationUpdates");
}

- (void) locationManager: (CLLocationManager*) locationManager
                didVisit: (CLVisit*) visit {
  auto locations = [NSArray arrayWithObject: locationManager.location];
  [self locationManager: locationManager didUpdateLocations: locations];
}

-       (void) locationManager: (CLLocationManager*) locationManager
  didChangeAuthorizationStatus: (CLAuthorizationStatus) status {
  // XXX(@jwerle): this is a legacy callback
  [self locationManagerDidChangeAuthorization: locationManager];
}

- (void) locationManagerDidChangeAuthorization: (CLLocationManager*) locationManager {
  using namespace ssc::runtime;
  auto activationCompletions = [NSArray arrayWithArray: self.locationObserver.activationCompletions];
  if (
  #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    locationManager.authorizationStatus == kCLAuthorizationStatusAuthorized ||
  #else
    locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedWhenInUse ||
  #endif
    locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedAlways
  ) {
    JSON::Object json = JSON::Object::Entries {
      {"state", "granted"}
    };

    self.locationObserver.geolocation->permissionChangeObservers.dispatch(json);
    self.locationObserver.isAuthorized = YES;
    for (id item in activationCompletions) {
      auto completion = (void (^)(BOOL)) item;
      completion(YES);
      [self.locationObserver.activationCompletions removeObject: item];
    #if !__has_feature(objc_arc)
      [completion release];
    #endif
    }
  } else {
    JSON::Object json = JSON::Object::Entries {
      {"state", locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined
        ? "prompt"
        : "denied"
      }
    };

    self.locationObserver.geolocation->permissionChangeObservers.dispatch(json);
    self.locationObserver.isAuthorized = NO;
    for (id item in activationCompletions) {
      auto completion = (void (^)(BOOL)) item;
      completion(NO);
      [self.locationObserver.activationCompletions removeObject: item];
    #if !__has_feature(objc_arc)
      [completion release];
    #endif
    }
  }
}
@end

#endif

namespace ssc::runtime::core::services {
  Geolocation::Geolocation (const Options& options)
    : core::Service(options),
      permissionChangeObservers()
  {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    this->locationObserver = [SSCLocationObserver new];
    this->locationPositionWatcher = [SSCLocationPositionWatcher new];
    this->locationManagerDelegate = [SSCLocationManagerDelegate new];
    this->locationObserver.geolocation = this;
  #endif
  }

  Geolocation::~Geolocation () {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
  #if !__has_feature(objc_arc)
    [this->locationObserver release];
    [this->locationPositionWatcher release];
    [this->locationManagerDelegate release];
  #endif
    this->locationObserver = nullptr;
    this->locationPositionWatcher = nullptr;
    this->locationManagerDelegate = nullptr;
  #endif
  }

  void Geolocation::getCurrentPosition (
    const String& seq,
    const Callback callback
  ) const {
    bool performedActivation = false;
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    performedActivation = [this->locationObserver getCurrentPositionWithCompletion: ^(NSError* error, CLLocation* location) {
      if (error != nullptr) {
        const auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
         );

        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "GeolocationPositionError"},
            {"message", message}
          }}
        };

        return callback(seq, json, QueuedResponse {});
      }

      const auto heading = this->locationObserver.locationManager.heading;
      const auto json = JSON::Object::Entries {
        {"coords", JSON::Object::Entries {
          {"latitude", location.coordinate.latitude},
          {"longitude", location.coordinate.longitude},
          {"altitude", location.altitude},
          {"accuracy", location.horizontalAccuracy},
          {"altitudeAccuracy", location.verticalAccuracy},
          {"floorLevel", location.floor.level},
          {"heading", heading.trueHeading},
          {"speed", location.speed}
        }}
      };

      callback(seq, json, QueuedResponse {});
    }];
  #endif

    if (!performedActivation) {
      const auto json = JSON::Object::Entries {
        {"err", JSON::Object::Entries {
          {"type", "GeolocationPositionError"},
          {"message", "Failed to get position"}
        }}
      };

      callback(seq, json, QueuedResponse {});
    }
  }

  void Geolocation::watchPosition (
    const String& seq,
    WatchID id,
    const Callback callback
  ) {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    const int identifier = [this->locationObserver watchPositionForIdentifier: id  completion: ^(NSError* error, CLLocation* location) {
      if (error != nullptr) {
        const auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
         );

        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "GeolocationPositionError"},
            {"message", message}
          }}
        };

        return callback(seq, json, QueuedResponse {});
      }

      const auto heading = this->locationObserver.locationManager.heading;
      const auto json = JSON::Object::Entries {
        {"watch", JSON::Object::Entries {
          {"identifier", identifier},
        }},
        {"coords", JSON::Object::Entries {
          {"latitude", location.coordinate.latitude},
          {"longitude", location.coordinate.longitude},
          {"altitude", location.altitude},
          {"accuracy", location.horizontalAccuracy},
          {"altitudeAccuracy", location.verticalAccuracy},
          {"floorLevel", location.floor.level},
          {"heading", heading.trueHeading},
          {"speed", location.speed}
        }}
      };

      callback("-1", json, QueuedResponse {});
    }];

    if (identifier != -1) {
      const auto json = JSON::Object::Entries {
        {"watch", JSON::Object::Entries {
          {"identifier", identifier}
        }}
      };

      return callback(seq, json, QueuedResponse {});
    }
  #endif

    const auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"type", "GeolocationPositionError"},
        {"message", "Failed to watch position"}
      }}
    };

    callback(seq, json, QueuedResponse {});
  }

  void Geolocation::clearWatch (
    const String& seq,
    WatchID id,
    const Callback callback
  ) {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    [this->locationObserver clearWatch: id];
  #endif

    callback(seq, JSON::Object {}, QueuedResponse {});
  }

  bool Geolocation::addPermissionChangeObserver (
    const PermissionChangeObserver& observer,
    const PermissionChangeObserver::Callback callback
  ) {
    return this->permissionChangeObservers.add(observer, callback);
  }

  bool Geolocation::removePermissionChangeObserver (const PermissionChangeObserver& observer) {
    return this->permissionChangeObservers.remove(observer);
  }
}
