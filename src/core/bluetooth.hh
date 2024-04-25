#ifndef SSC_CORE_BLUETOOTH_H
#define SSC_CORE_BLUETOOTH_H

#include "json.hh"
#include "platform.hh"
#include "post.hh"
#include "types.hh"

#if SSC_PLATFORM_APPLE
@interface SSCBluetoothController : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate
>
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) NSMutableDictionary* services;
@property (strong, nonatomic) NSMutableDictionary* characteristics;
@property (strong, nonatomic) NSMutableDictionary* serviceMap;
- (void) startAdvertising;
- (void) startScanning;
- (id) init;
@end
#endif

namespace SSC {
  class Core;
  class Bluetooth {
    public:
      using SendFunction = Function<void(const String, JSON::Any, Post)>;
      using EmitFunction = Function<void(const String, JSON::Any)>;
      using Callback = Function<void(String, JSON::Any)>;

    #if SSC_PLATFORM_APPLE
      SSCBluetoothController* controller = nullptr;
    #endif

      Core *core = nullptr;
      SendFunction sendFunction;
      EmitFunction emitFunction;

      Bluetooth ();
      ~Bluetooth ();
      bool send (const String& seq, JSON::Any json, Post post);
      bool send (const String& seq, JSON::Any json);
      bool emit (const String& seq, JSON::Any json);
      void startScanning ();
      void publishCharacteristic (
        const String& seq,
        char* bytes,
        size_t size,
        const String& serviceId,
        const String& characteristicId,
        Callback callback
      );

      void subscribeCharacteristic (
        const String& seq,
        const String& serviceId,
        const String& characteristicId,
        Callback callback
      );

      void startService (
        const String& seq,
        const String& serviceId,
        Callback callback
      );
  };
}
#endif
