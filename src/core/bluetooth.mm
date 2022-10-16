#include "core.hh"
#include "../ipc/ipc.hh"

using namespace SSC;

@implementation SSCBluetoothDelegate
// - (void) disconnect {
// }

// - (void) updateRssi {
//  debug("CoreBluetooth: updateRssi");
// }

- (void) stopAdvertising {
  debug("CoreBluetooth: stopAdvertising");

  [self.peripheralManager stopAdvertising];
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

  self.bridge.router->emit("bluetooth", JSON::Object(json).str());
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

    self.bridge.router->emit("bluetooth", JSON::Object(json).str());
    return;
  }

  debug("CoreBluetooth didStartAdvertising: %@", _serviceMap);
}

-   (void) peripheralManager: (CBPeripheralManager*) peripheralManager
                     central: (CBCentral*) central
didSubscribeToCharacteristic: (CBCharacteristic*) characteristic {
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

- (void) initBluetooth {
  _peripherals = [[NSMutableArray alloc] init];
  _serviceMap = [[NSMutableDictionary alloc] init];
  _services = [[NSMutableDictionary alloc] init];
  _characteristics = [[NSMutableDictionary alloc] init];
  _centralManager = [[CBCentralManager alloc] initWithDelegate: self queue: nil];
  _peripheralManager = [[CBPeripheralManager alloc] initWithDelegate: self queue: nil options: nil];
}

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

- (void) peripheralManager: (CBPeripheralManager*) peripheral
             didAddService: (CBService*) service
                     error: (NSError *) error
{
  [self startAdvertising];
}

-(void) subscribeCharacteristic: (String) seq
                            sid: (String) sid
                            cid: (String) cid
{
  NSString* ssid = [NSString stringWithUTF8String: sid.c_str()];
  NSString* scid = [NSString stringWithUTF8String: cid.c_str()];

  CBMutableCharacteristic* ch = [[CBMutableCharacteristic alloc]
    initWithType: [CBUUID UUIDWithString: scid]
      properties: (CBCharacteristicPropertyNotify | CBCharacteristicPropertyRead | CBCharacteristicPropertyWrite)
           value: nil
     permissions: (CBAttributePermissionsReadable | CBAttributePermissionsWriteable)
  ];

  if (!_characteristics[ssid]) {
    _characteristics[ssid] = [NSMutableSet setWithObject: ch];
  } else {
    [_characteristics[ssid] addObject: ch];
  }

  if (!_serviceMap[ssid]) {
    _serviceMap[ssid] = [NSMutableSet setWithObject: [CBUUID UUIDWithString: scid]];
  } else {
    [_serviceMap[ssid] addObject: [CBUUID UUIDWithString: scid]];
  }

  CBMutableService* service = _services[ssid];

  if (!service) {
    auto sUUID = [CBUUID UUIDWithString: ssid];
    service = [[CBMutableService alloc] initWithType: sUUID primary: YES];

    service.characteristics = _characteristics[ssid];

    [_services setValue: service forKey: ssid];
    [_peripheralManager addService: service];
  }

  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"serviceId", sid},
      {"characteristicId", cid},
      {"message", "Started"}
    }}
  };

  self.bridge.router->send(seq, JSON::Object(json).str(), Post{});
}

- (void) publishCharacteristic: (String) seq
                           buf: (char*) buf
                           len: (int) len
                           sid: (String) sid
                           cid: (String) cid
{
  NSString* ssid = [NSString stringWithUTF8String: sid.c_str()];
  NSString* scid = [NSString stringWithUTF8String: cid.c_str()];

  auto sUUID = [CBUUID UUIDWithString: ssid];
  auto cUUID = [CBUUID UUIDWithString: scid];

  if (!_serviceMap[ssid]) {
    auto msg = format(R"MSG({ "err": { "message": "invalid serviceId" } })MSG");
    [self.bridge send: seq msg: msg post: Post{}];
    return;
  }

  if (![_serviceMap[ssid] containsObject: cUUID]) {
    auto msg = format(R"MSG({ "err": { "message": "invalid characteristicId" } })MSG");
    [self.bridge send: seq msg: msg post: Post{}];
    return;
  }

  [self startScanning]; // noop if already scanning.

  if (len == 0) {
    debug("CoreBluetooth: characteristic added");
    return;
  }

  // TODO (@heapwolf) enforce max message legth and split into multiple writes.
  // NSInteger amountToSend = self.dataToSend.length - self.sendDataIndex;
	// if (amountToSend > 512) amountToSend = 512;

  auto* data = [NSData dataWithBytes: buf length: len];

  CBMutableCharacteristic* characteristic;
  for (CBMutableCharacteristic* ch in _characteristics[ssid]) {
    if ([ch.UUID isEqual: cUUID]) characteristic = ch;
  }

  characteristic.value = data;

  auto didWrite = [
    _peripheralManager
      updateValue: data
      forCharacteristic: characteristic
      onSubscribedCentrals: nil
  ];

  if (!didWrite) {
    debug("CoreBluetooth: did not write");
    return;
  }

  debug("CoreBluetooth: did write '%@' %@", data, characteristic);
}

// start advertising and scanning for a new service
- (void) startService: (String) seq sid: (String) sid {
  [self startScanning];

  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"serviceId", sid},
      {"message", "Started"}
    }}
  };

  self.bridge.router->send(seq, JSON::Object(json).str(), Post{});
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

  self.bridge.router->emit("bluetooth", JSON::Object(json).str());

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

  self.bridge.router->emit("bluetooth", JSON::Object(json).str());
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
  auto headers = Headers {{
    {"content-type", "application/octet-stream"},
    {"content-length", length}
  }};

  Post post = {0};
  post.id = rand64();
  post.body = bytes;
  post.length = length;
  post.headers = headers.str();

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

  self.bridge.router->send("-1", JSON::Object(json).str(), post);
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
