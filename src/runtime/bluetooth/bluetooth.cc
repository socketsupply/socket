#include "../ipc.hh"
#include "../http.hh"
#include "../json.hh"
#include "../debug.hh"
#include "../crypto.hh"
#include "../queued_response.hh"

#include "../bluetooth.hh"

using namespace ssc::runtime;
using ssc::runtime::crypto::rand64;

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCBluetoothController ()
@property (nonatomic) Bluetooth* bluetooth;
@end

@implementation SSCBluetoothController
- (id) init {
  self = [super init];

  _services = [[NSMutableDictionary alloc] init];
  _serviceMap = [[NSMutableDictionary alloc] init];
  _peripherals = [[NSMutableArray alloc] init];
  _characteristics = [[NSMutableDictionary alloc] init];

  _centralManager = [[CBCentralManager alloc]
    initWithDelegate: self
               queue: nil
  ];

  _peripheralManager = [[CBPeripheralManager alloc]
    initWithDelegate: self
               queue: nil
             options: nil
  ];

  return self;
}

// - (void) disconnect {
// }

// - (void) updateRssi {
//  debug("CoreBluetooth: updateRssi");
// }

- (void) startAdvertising {
  if (_peripheralManager.isAdvertising) return;

  NSArray* keys = [_serviceMap allKeys];
  if ([keys count] == 0) return;


  NSMutableArray* uuids = [[NSMutableArray alloc] init];
  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  [_peripheralManager startAdvertising: @{CBAdvertisementDataServiceUUIDsKey: [uuids copy]}];
}

- (void) stopAdvertising {
  debug("CoreBluetooth: stopAdvertising");

  [self.peripheralManager stopAdvertising];
}

- (void) startScanning {
  NSArray* keys = [_serviceMap allKeys];
  if ([keys count] == 0) return;

  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  debug("CoreBluetooth: startScanning %@", uuids);

  [_centralManager
    scanForPeripheralsWithServices: [uuids copy]
    options: @{CBCentralManagerScanOptionAllowDuplicatesKey: @(YES)}
  ];
}

- (void) peripheralManagerDidUpdateState: (CBPeripheralManager*) peripheral {
  String state = "Unknown state";
  String message = "Unknown state";

  switch (peripheral.state) {
    case CBManagerStatePoweredOff:
      message = "CoreBluetooth BLE hardware is powered off.";
      state = "CBManagerStatePoweredOff";
      break;

    case CBManagerStatePoweredOn:
      // [self startAdvertising];
      [self startScanning];
      message = "CoreBluetooth BLE hardware is powered on and ready.";
      state = "CBManagerStatePoweredOn";
      break;

    case CBManagerStateUnauthorized:
      message = "CoreBluetooth BLE state is unauthorized.";
      state = "CBManagerStateUnauthorized";
      break;

    case CBManagerStateUnknown:
      message = "CoreBluetooth BLE state is unknown.";
      state = "CBManagerStateUnknown";
      break;

    case CBManagerStateUnsupported:
      message = "CoreBluetooth BLE hardware is unsupported on this platform.";
      state = "CBManagerStateUnsupported";
      break;

    default:
      break;
  }

  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"event", "state"},
      {"state", message}
    }}
  };

  auto result = ipc::Result(json);
  self.bluetooth->emit("bluetooth", result.json());
}

- (void) peripheralManagerDidStartAdvertising: (CBPeripheralManager*) peripheral
                                        error: (NSError*) error
{
  if (error) {
    debug("CoreBluetooth: %@", error);
    auto desc = String([error.debugDescription UTF8String]);
    std::replace(desc.begin(), desc.end(), '"', '\''); // Secure

    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"event", "peripheralManagerDidStartAdvertising"},
        {"message", desc}
      }}
    };

    auto result = ipc::Result(json);
    self.bluetooth->emit("bluetooth", result.json());
    return;
  }

  debug("CoreBluetooth didStartAdvertising: %@", _serviceMap);
}

-    (void) peripheralManager: (CBPeripheralManager*) peripheralManager
                      central: (CBCentral*) central
 didSubscribeToCharacteristic: (CBCharacteristic*) characteristic
{
  debug("CoreBluetooth: didSubscribeToCharacteristic");
}

- (void) centralManagerDidUpdateState: (CBCentralManager*) central {
  switch (central.state) {
    case CBManagerStatePoweredOff:
    case CBManagerStateResetting:
    case CBManagerStateUnauthorized:
    case CBManagerStateUnknown:
    case CBManagerStateUnsupported:
      break;

    case CBManagerStatePoweredOn:
      [self startScanning];
    break;
  }
}

- (void) peripheralManager: (CBPeripheralManager*) peripheral
             didAddService: (CBService*) service
                     error: (NSError *) error
{
  [self startAdvertising];
}

- (void) peripheralManager: (CBPeripheralManager*) peripheral
     didReceiveReadRequest: (CBATTRequest*) request
{
  CBMutableCharacteristic* ch;

  for (NSString* key in _services) {
    NSMutableSet* characteristics = _characteristics[key];

    for (CBMutableCharacteristic* c in characteristics) {
      if (c.UUID == request.characteristic.UUID) {
        ch = c;
        break;
      }
    }
  }

  if (!ch) {
    debug("CoreBluetooth: No local characteristic found for request");
    return;
  }

  request.value = ch.value;
  [_peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];

  [self startScanning];
}

- (void) centralManager: (CBCentralManager*) central
  didDiscoverPeripheral: (CBPeripheral*) peripheral
      advertisementData: (NSDictionary*) advertisementData
                   RSSI: (NSNumber*) RSSI
{
  if (peripheral.identifier == nil || peripheral.name == nil) {
    [self.peripherals addObject: peripheral];

    NSTimeInterval _scanTimeout = 0.5;
    [NSTimer timerWithTimeInterval: _scanTimeout repeats: NO block:^(NSTimer* timer) {
      debug("CoreBluetooth: reconnecting");
      [self->_centralManager connectPeripheral: peripheral options: nil];
    }];
    return;
  }

  auto isConnected = peripheral.state != CBPeripheralStateDisconnected;
  auto isKnown = [_peripherals containsObject: peripheral];

  if (isKnown && isConnected) {
    return;
  }

  if (!isKnown) {
    peripheral.delegate = self;
    [self.peripherals addObject: peripheral];
  }

  [_centralManager connectPeripheral: peripheral options: nil];

  for (CBService *service in peripheral.services) {
    NSString* key = service.UUID.UUIDString;
    NSArray* characteristics = [_characteristics[key] allObjects];
    for (CBCharacteristic* ch in characteristics) {
      [peripheral readValueForCharacteristic: ch];
    }
  }
}

- (void) centralManager: (CBCentralManager*) central
   didConnectPeripheral: (CBPeripheral*) peripheral
{
  debug("CoreBluetooth: didConnectPeripheral");
  peripheral.delegate = self;

  NSArray* keys = [_serviceMap allKeys];
  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  String uuid = String([peripheral.identifier.UUIDString UTF8String]);
  String name = String([peripheral.name UTF8String]);

  if (uuid.size() == 0 || name.size() == 0) {
    debug("device has no meta information");
    return;
  }

  auto json = JSON::Object::Entries {
    {"data" , JSON::Object::Entries {
      {"event", "connect"},
      {"name", name},
      {"uuid", uuid}
    }}
  };

  auto result = ipc::Result(json);
  self.bluetooth->emit("bluetooth", result.json());

  [peripheral discoverServices: uuids];
}

-  (void) peripheral: (CBPeripheral*) peripheral
 didDiscoverServices: (NSError*) error {
  if (error) {
    debug("CoreBluetooth: peripheral:didDiscoverService:error: %@", error);
    return;
  }

  for (CBService *service in peripheral.services) {
    NSString* key = service.UUID.UUIDString;
    NSArray* uuids = [_serviceMap[key] allObjects];

    debug("CoreBluetooth: peripheral:didDiscoverServices withChracteristics: %@", uuids);
    [peripheral discoverCharacteristics: [uuids copy] forService: service];
  }
}

-                   (void) peripheral: (CBPeripheral*) peripheral
 didDiscoverCharacteristicsForService: (CBService*) service
                                error: (NSError*) error
{
  if (error) {
    debug("CoreBluetooth: peripheral:didDiscoverCharacteristicsForService:error: %@", error);
    return;
  }

  NSString* key = service.UUID.UUIDString;
  NSArray* uuids = [[_serviceMap[key] allObjects] copy];

  for (CBCharacteristic* characteristic in service.characteristics) {
    for (CBUUID* cUUID in uuids) {
      if ([characteristic.UUID isEqual: cUUID]) {
        [peripheral setNotifyValue: YES forCharacteristic: characteristic];
        [peripheral readValueForCharacteristic: characteristic];
      }
    }
  }
}

- (void) peripheralManagerIsReadyToUpdateSubscribers: (CBPeripheralManager*) peripheral {
  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"source", "bluetooth"},
      {"message", "peripheralManagerIsReadyToUpdateSubscribers"},
      {"event", "status"}
    }}
  };

  auto result = ipc::Result(json);
  self.bluetooth->emit("bluetooth", result.json());
}

-              (void) peripheral: (CBPeripheral*) peripheral
 didUpdateValueForCharacteristic: (CBCharacteristic*) characteristic
                           error: (NSError*) error
{
  if (error) {
    debug("ERROR didUpdateValueForCharacteristic: %@", error);
    return;
  }

  if (!characteristic.value || characteristic.value.length == 0) return;

  String uuid = "";
  String name = "";

  if (peripheral.identifier != nil) {
    uuid = [peripheral.identifier.UUIDString UTF8String];
  }

  if (peripheral.name != nil) {
    name = [peripheral.name UTF8String];
    std::replace(name.begin(), name.end(), '\n', ' '); // Secure
  }

  String characteristicId = [characteristic.UUID.UUIDString UTF8String];
  String sid = "";

  for (NSString* ssid in [_serviceMap allKeys]) {
    if ([_serviceMap[ssid] containsObject: characteristic.UUID]) {
      sid = [ssid UTF8String];
    }
  }

  auto bytes = (char*) characteristic.value.bytes;
  auto length = (int) characteristic.value.length;
  auto headers = http::Headers {{
    {"content-type", "application/octet-stream"},
    {"content-length", length}
  }};

  QueuedResponse queuedResponse = {0};
  queuedResponse.id = rand64();
  queuedResponse.headers = headers;

  if (bytes != nullptr && length > 0) {
    queuedResponse.body = std::make_shared<char[]>(length);
    queuedResponse.length = length;
    memcpy(queuedResponse.body.get(), bytes, length);
  }

  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"event", "data"},
      {"source", "bluetooth"},
      {"characteristicId", characteristicId},
      {"serviceId", sid},
      {"name", name},
      {"uuid", uuid}
    }}
  };

  auto result = ipc::Result(json);
  self.bluetooth->send(result.seq, result.json(), queuedResponse);
}

-                          (void) peripheral: (CBPeripheral*) peripheral
 didUpdateNotificationStateForCharacteristic: (CBCharacteristic*) characteristic
                                       error: (NSError*) error
{
  // TODO
}

-     (void) centralManager: (CBCentralManager*) central
 didFailToConnectPeripheral: (CBPeripheral*) peripheral
                      error: (NSError*) error
{
  // if (error != nil) {
  //  debug("CoreBluetooth: failed to connect %@", error.debugDescription);
  //  return;
  // }

  NSTimeInterval _scanTimeout = 0.5;

  [NSTimer timerWithTimeInterval: _scanTimeout repeats: NO block:^(NSTimer* timer) {
    [self->_centralManager connectPeripheral: peripheral options: nil];

    // if ([_peripherals containsObject: peripheral]) {
    //  [_peripherals removeObject: peripheral];
    // }
  }];
}

-  (void) centralManager: (CBCentralManager*) central
 didDisconnectPeripheral: (CBPeripheral*) peripheral
                   error: (NSError*) error
{
  [_centralManager connectPeripheral: peripheral options: nil];
  if (error != nil) {
    debug("CoreBluetooth: device did disconnect %@", error.debugDescription);
    return;
  }
}
@end
#endif

namespace ssc::runtime::bluetooth {
  Bluetooth::Bluetooth () {}

  Bluetooth::~Bluetooth () {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->controller != nullptr) {
      #if !__has_feature(objc_arc)
      [this->controller release];
      #endif
    }
    #endif
  }

  bool Bluetooth::send (const String& seq, JSON::Any json, QueuedResponse queuedResponse) {
    if (this->sendHandler != nullptr) {
      this->sendHandler(seq, json, queuedResponse);
      return true;
    }
    return false;
  }

  bool Bluetooth::send (const String& seq, JSON::Any json) {
    return this->send(seq, json, QueuedResponse{});
  }

  bool Bluetooth::emit (const String& seq, JSON::Any json) {
    if (this->emitHandler != nullptr) {
      this->emitHandler(seq, json);
      return true;
    }
    return false;
  }

  void Bluetooth::startScanning () {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->controller == nullptr) {
      this->controller = [SSCBluetoothController new];
    }

    [this->controller setBluetooth: this];
    if (this->controller != nullptr) {
      [this->controller startScanning];
    }
    #endif
  }

  void Bluetooth::publishCharacteristic (
    const char *bytes,
    size_t size,
    const ServiceID& serviceId,
    const CharacteristicID& characteristicId,
    const Callback callback
  ) {
    this->publishCharacteristic("-1", bytes, size, serviceId, characteristicId, callback);
  }

  void Bluetooth::publishCharacteristic (
    const ipc::Message::Seq& seq,
    const char *bytes,
    size_t size,
    const ServiceID& serviceId,
    const CharacteristicID& characteristicId,
    const Callback callback
  ) {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (serviceId.size() != 36) {
      callback(seq, JSON::Object::Entries {
        {"source", "bluetooth.publish"},
        {"err", JSON::Object::Entries {
          {"message", "Invalid service id"}
        }}
      });
      return;
    }

    if (characteristicId.size() != 36) {
      callback(seq, JSON::Object::Entries {
        {"source", "bluetooth.publish"},
        {"err", JSON::Object::Entries {
          {"message", "Invalid characteristic id"}
        }}
      });
      return;
    }

    if (bytes == nullptr || size <= 0) {
      callback(seq, JSON::Object::Entries {
        {"source", "bluetooth.publish"},
        {"err", JSON::Object::Entries {
          {"message", "Missing bytes in publish"}
        }}
      });
      return;
    }

    auto ssid = [NSString stringWithUTF8String: serviceId.c_str()];
    auto scid = [NSString stringWithUTF8String: characteristicId.c_str()];

    auto peripheralManager = [this->controller peripheralManager];
    auto characteristics = [this->controller characteristics];
    auto serviceMap = [this->controller serviceMap];
    auto cUUID = [CBUUID UUIDWithString: scid];

    if (!serviceMap[ssid]) {
      auto json = JSON::Object::Entries {
        {"source", "bluetooth.publish"},
        {"err", JSON::Object::Entries {
          {"message", "Invalid service id"}
        }}
      };

      callback(seq, json);
      return;
    }

    if (![serviceMap[ssid] containsObject: cUUID]) {
      auto json = JSON::Object::Entries {
        {"source", "bluetooth.publish"},
        {"err", JSON::Object::Entries {
          {"message", "Invalid characteristic id"}
        }}
      };

      callback(seq, json);
      return;
    }

    [this->controller startScanning]; // noop if already scanning.

    if (size == 0) {
      debug("CoreBluetooth: characteristic added");
      return;
    }

    // TODO (@heapwolf) enforce max message legth and split into multiple writes.
    // NSInteger amountToSend = self.dataToSend.length - self.sendDataIndex;
	  // if (amountToSend > 512) amountToSend = 512;
    auto data = [NSData dataWithBytes: bytes length: size];

    CBMutableCharacteristic* characteristic;
    for (CBMutableCharacteristic* ch in characteristics[ssid]) {
      if ([ch.UUID isEqual: cUUID]) characteristic = ch;
    }

    characteristic.value = data;

    auto didWrite = [peripheralManager
               updateValue: data
         forCharacteristic: characteristic
      onSubscribedCentrals: nil
    ];

    if (!didWrite) {
      debug("CoreBluetooth: did not write");
      return;
    }

    debug("CoreBluetooth: did write '%@' %@", data, characteristic);
    callback(seq, JSON::Object::Entries {
      {"data", JSON::Object::Entries {
        {"serviceId", serviceId},
        {"characteristicId", characteristicId},
        {"message", "Published to characteristic"}
      }}
    });
    #else
    callback(seq, JSON::Object::Entries {
      {"source", "bluetooth.publish"},
      {"err", JSON::Object::Entries {
        {"type", "NotSupportedError"},
        {"message", "Not supported"}
      }}
    });
    #endif
  }

  void Bluetooth::subscribeCharacteristic (
    const ServiceID& serviceId,
    const CharacteristicID& characteristicId,
    const Callback callback
  ) {
    this->subscribeCharacteristic("-1", serviceId, characteristicId, callback);
  }

  void Bluetooth::subscribeCharacteristic (
    const ipc::Message::Seq& seq,
    const ServiceID& serviceId,
    const CharacteristicID& characteristicId,
    const Callback callback
  ) {
    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"serviceId", serviceId},
        {"message", "Failed to subscribe to characteristic"}
      }}
    };

    #if SOCKET_RUNTIME_PLATFORM_APPLE
    auto ssid = [NSString stringWithUTF8String: serviceId.c_str()];
    auto scid = [NSString stringWithUTF8String: characteristicId.c_str()];

    auto peripheralManager = [this->controller peripheralManager];
    auto characteristics = [this->controller characteristics];
    auto serviceMap = [this->controller serviceMap];
    auto services = [this->controller services];

    auto  ch = [[CBMutableCharacteristic alloc]
      initWithType: [CBUUID UUIDWithString: scid]
        properties: (CBCharacteristicPropertyNotify | CBCharacteristicPropertyRead | CBCharacteristicPropertyWrite)
             value: nil
       permissions: (CBAttributePermissionsReadable | CBAttributePermissionsWriteable)
    ];

    if (!characteristics[ssid]) {
      characteristics[ssid] = [NSMutableSet setWithObject: ch];
    } else {
      [characteristics[ssid] addObject: ch];
    }

    if (!serviceMap[ssid]) {
      serviceMap[ssid] = [NSMutableSet setWithObject: [CBUUID UUIDWithString: scid]];
    } else {
      [serviceMap[ssid] addObject: [CBUUID UUIDWithString: scid]];
    }

    CBMutableService* service = services[ssid];

    if (!service) {
      auto sUUID = [CBUUID UUIDWithString: ssid];
      service = [[CBMutableService alloc] initWithType: sUUID primary: YES];

      service.characteristics = characteristics[ssid];

      [services setValue: service forKey: ssid];
      [peripheralManager addService: service];
    }

    json = JSON::Object::Entries {
      {"data", JSON::Object::Entries {
        {"serviceId", serviceId},
        {"characteristicId", characteristicId},
        {"message", "Subscribed to characteristic"}
      }}
    };
    #endif

    callback(seq, json);
  }

  void Bluetooth::startService (
    const ServiceID& serviceId,
    const Callback callback
  ) {
    this->startService("-1", serviceId, callback);
  }

  void Bluetooth::startService (
    const ipc::Message::Seq& seq,
    const ServiceID& serviceId,
    const Callback callback
  ) {
    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"serviceId", serviceId},
        {"message", "Failed to start service"}
      }}
    };

    #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->controller != nullptr) {
      [this->controller startScanning];
      json = JSON::Object::Entries {
        {"data", JSON::Object::Entries {
          {"serviceId", serviceId},
          {"message", "Service started"}
        }}
      };
    }
    #endif

    callback(seq, json);
  }
}
