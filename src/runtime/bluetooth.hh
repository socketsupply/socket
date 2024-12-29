#ifndef SOCKET_RUNTIME_RUNTIME_BLUETOOTH_H
#define SOCKET_RUNTIME_RUNTIME_BLUETOOTH_H

#include "ipc.hh"
#include "json.hh"
#include "platform.hh"
#include "queued_response.hh"

#if SOCKET_RUNTIME_PLATFORM_APPLE
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

namespace ssc::runtime::bluetooth {
  class Bluetooth {
    public:
      using SendHandler = Function<void(const String, JSON::Any, QueuedResponse)>;
      using EmitHandler = Function<void(const String, JSON::Any)>;
      using Callback = Function<void(String, JSON::Any)>;
      using CharacteristicID = String;
      using ServiceID = String;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCBluetoothController* controller = nullptr;
    #endif

      SendHandler sendHandler;
      EmitHandler emitHandler;

      Bluetooth ();
      ~Bluetooth ();

      bool send (const ipc::Message::Seq&, JSON::Any, QueuedResponse);
      bool send (const ipc::Message::Seq&, JSON::Any json);
      bool emit (const ipc::Message::Seq&, JSON::Any json);
      void startScanning ();
      void publishCharacteristic (
        const ipc::Message::Seq&,
        const char*,
        size_t,
        const ServiceID&,
        const CharacteristicID&,
        const Callback
      );

      void publishCharacteristic (
        const char*,
        size_t,
        const ServiceID&,
        const CharacteristicID&,
        const Callback
      );

      void subscribeCharacteristic (
        const ipc::Message::Seq&,
        const ServiceID&,
        const CharacteristicID&,
        const Callback
      );

      void subscribeCharacteristic (
        const ServiceID&,
        const CharacteristicID&,
        const Callback
      );

      void startService (
        const ipc::Message::Seq&,
        const ServiceID&,
        const Callback
      );

      void startService (
        const ServiceID&,
        const Callback
      );
  };
}
#endif
