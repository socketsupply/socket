#ifndef SOCKET_RUNTIME_CORE_SERVICES_GEOLOCATION_H
#define SOCKET_RUNTIME_CORE_SERVICES_GEOLOCATION_H

#include "../../core.hh"

namespace ssc::runtime::core::services {
  class Geolocation;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@class SSCLocationObserver;

@interface SSCLocationManagerDelegate : NSObject<CLLocationManagerDelegate>
@property (nonatomic, strong) SSCLocationObserver* locationObserver;

- (id) initWithLocationObserver: (SSCLocationObserver*) locationObserver;

- (void) locationManager: (CLLocationManager*) locationManager
        didFailWithError: (NSError*) error;

- (void) locationManager: (CLLocationManager*) locationManager
      didUpdateLocations: (NSArray<CLLocation*>*) locations;

- (void)            locationManager: (CLLocationManager*) locationManager
  didFinishDeferredUpdatesWithError: (NSError*) error;

- (void) locationManagerDidPauseLocationUpdates: (CLLocationManager*) locationManager;
- (void) locationManagerDidResumeLocationUpdates: (CLLocationManager*) locationManager;
- (void) locationManager: (CLLocationManager*) locationManager
                didVisit: (CLVisit*) visit;

-       (void) locationManager: (CLLocationManager*) locationManager
  didChangeAuthorizationStatus: (CLAuthorizationStatus) status;
- (void) locationManagerDidChangeAuthorization: (CLLocationManager*) locationManager;
@end

@interface SSCLocationPositionWatcher : NSObject
@property (nonatomic, assign) NSInteger identifier;
@property (nonatomic, assign) void(^completion)(CLLocation*);
+ (SSCLocationPositionWatcher*) positionWatcherWithIdentifier: (NSInteger) identifier
                                                   completion: (void (^)(CLLocation*)) completion;
@end

@interface SSCLocationObserver : NSObject
@property (nonatomic, retain) CLLocationManager* locationManager;
@property (nonatomic, retain) SSCLocationManagerDelegate* delegate;
@property (atomic, retain) NSMutableArray* activationCompletions;
@property (atomic, retain) NSMutableArray* locationRequestCompletions;
@property (atomic, retain) NSMutableArray* locationWatchers;
W@property (nonatomic) ssc::runtime::core::services::Geolocation* geolocation;
@property (atomic, assign) BOOL isAuthorized;
- (BOOL) attemptActivation;
- (BOOL) attemptActivationWithCompletion: (void (^)(BOOL)) completion;
- (BOOL) getCurrentPositionWithCompletion: (void (^)(NSError*, CLLocation*)) completion;
- (int) watchPositionForIdentifier: (NSInteger) identifier
                        completion: (void (^)(NSError*, CLLocation*)) completion;
- (BOOL) clearWatch: (NSInteger) identifier;
@end
#endif

namespace ssc::runtime::core::services {
  class Geolocation : public core::Service {
    public:
      using WatchID = uint64_t;
      using PermissionChangeObserver = Observer<JSON::Object>;
      using PermissionChangeObservers = Observers<PermissionChangeObserver>;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCLocationObserver* locationObserver = nullptr;
      SSCLocationPositionWatcher* locationPositionWatcher = nullptr;
      SSCLocationManagerDelegate* locationManagerDelegate = nullptr;
    #endif

      PermissionChangeObservers permissionChangeObservers;

      Geolocation (const Options&);
      ~Geolocation ();

      void getCurrentPosition (
        const String& seq,
        const Callback callback
      ) const;

      void watchPosition (
        const String& seq,
        const WatchID id,
        const Callback callback
      );

      void clearWatch (
        const String& seq,
        const WatchID id,
        const Callback callback
      );

      bool removePermissionChangeObserver (
        const PermissionChangeObserver& observer
      );

      bool addPermissionChangeObserver (
        const PermissionChangeObserver& observer,
        const PermissionChangeObserver::Callback callback
      );
  };
}
#endif
