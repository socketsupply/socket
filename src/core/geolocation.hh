#ifndef SSC_CORE_GEOLOCATION_H
#define SSC_CORE_GEOLOCATION_H

#include "platform.hh"
#include "types.hh"
#include "module.hh"

namespace SSC {
  // forward
  class Core;
  class Geolocation;
}

#if SSC_PLATFORM_APPLE
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
@property (nonatomic) SSC::Geolocation* geolocation;
@property (atomic, assign) BOOL isAuthorized;
- (BOOL) attemptActivation;
- (BOOL) attemptActivationWithCompletion: (void (^)(BOOL)) completion;
- (BOOL) getCurrentPositionWithCompletion: (void (^)(NSError*, CLLocation*)) completion;
- (int) watchPositionForIdentifier: (NSInteger) identifier
                        completion: (void (^)(NSError*, CLLocation*)) completion;
- (BOOL) clearWatch: (NSInteger) identifier;
@end
#endif

namespace SSC {
  class Geolocation : public Module {
    public:
      using PermissionChangeObserver = Module::Observer<JSON::Object>;
      using PermissionChangeObservers = Module::Observers<PermissionChangeObserver>;

    #if SSC_PLATFORM_APPLE
      SSCLocationObserver* locationObserver = nullptr;
      SSCLocationPositionWatcher* locationPositionWatcher = nullptr;
      SSCLocationManagerDelegate* locationManagerDelegate = nullptr;
    #endif

      PermissionChangeObservers permissionChangeObservers;

      Geolocation (Core* core);
      ~Geolocation ();
      void getCurrentPosition (const String& seq, const Module::Callback callback);
      void watchPosition (const String& seq, uint64_t id, const Module::Callback callback);
      void clearWatch (
        const String& seq,
        uint64_t id,
        const Module::Callback callback
      );

      bool removePermissionChangeObserver (const PermissionChangeObserver& observer);
      bool addPermissionChangeObserver (
        const PermissionChangeObserver& observer,
        const PermissionChangeObserver::Callback callback
      );
  };
}
#endif
