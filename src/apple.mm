//
// Mixed-into ios.mm and mac.hh by #include. This file
// expects WV to be defined before it's included. 
//
@interface BluetoothDelegate : NSObject<
	CBCentralManagerDelegate,
	CBPeripheralManagerDelegate,
	CBPeripheralDelegate>
@property (strong, nonatomic) WV* webview;
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) CBMutableService* service;
@property (strong, nonatomic) CBMutableCharacteristic* characteristic;
@property (strong, nonatomic) NSString* channelId;
@property (strong, nonatomic) NSString* serviceId;
- (void) initBluetooth;
- (void) setChannelId: (std::string)str;
@end

@implementation BluetoothDelegate
WV* webview;

- (void)disconnect {
  NSLog(@"BBB disconnect");
  // throw new Error('disconnect is not supported on OS X!');
}
- (void)updateRssi {
  NSLog(@"BBB updateRssi");
}

- (void) startAdvertisingIBeacon:(NSData *)data {
  NSLog(@"BBB startAdvertisingIBeacon:%@", data);
}

- (void) stopAdvertising {
  NSLog(@"BBB stopAdvertising");

  [self.peripheralManager stopAdvertising];
}

- (void) peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral {
    std::string state = "Unknown state";
    std::string message = "Unknown state";

    switch (peripheral.state) {
      case CBManagerStatePoweredOff:
        message = "CoreBluetooth BLE hardware is powered off.";
        state = "CBManagerStatePoweredOff";
        break;

      case CBManagerStatePoweredOn:
        [self startBluetooth];
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

    auto msg = SSC::format(R"JSON({
      "value": {
        "data": {
          "message": "$S"
          "state": "$S"
        }
      }
    })JSON", message, state);

    [self.webview emit: "local-network" msg: msg];

    NSLog(@"%@", [NSString stringWithUTF8String: msg.c_str()]);
}

- (void) peripheralManagerDidStartAdvertising: (CBPeripheralManager*)peripheral error:(nullable NSError *)error {
  NSLog(@"BBB peripheralManagerDidStartAdvertising: %@", peripheral.description);
  if (error) {
      NSLog(@"BBB Error advertising: %@", [error localizedDescription]);
  }
}

- (void) peripheralManager:(CBPeripheralManager*)peripheral central:(CBCentral*)central didSubscribeToCharacteristic:(CBCharacteristic*)characteristic {
  NSLog(@"BBB didSubscribeToCharacteristic");
}

- (void) peripheralManager:(CBPeripheralManager *)peripheral didPublishL2CAPChannel:(CBL2CAPPSM)PSM error:(nullable NSError *)error {
    NSLog(@"BBB didPublishL2CAPChannel");
}

- (void) peripheralManager:(CBPeripheralManager *)peripheral didUnpublishL2CAPChannel:(CBL2CAPPSM)PSM error:(nullable NSError *)error {
    NSLog(@"BBB didUnpublishL2CAPChannel");
}

- (void) peripheralManager:(CBPeripheralManager *)peripheral didOpenL2CAPChannel:(nullable CBL2CAPChannel *)channel error:(nullable NSError *)error {
    NSLog(@"BBB didOpenL2CAPChannel");
}

- (void) centralManagerDidUpdateState: (CBCentralManager*)central {
  NSLog(@"BBB centralManagerDidUpdateState");
}

- (void) setChannelId: (std::string)str {
  NSString* channelId = [NSString stringWithUTF8String: str.c_str()];
  _channelId = channelId;
}

- (void) initBluetooth {
  _centralManager = [[CBCentralManager alloc] initWithDelegate: self queue: nil];
  _peripheralManager = [[CBPeripheralManager alloc] initWithDelegate: self queue: nil options: nil];
  _peripherals = [NSMutableArray array];
	_channelId = @"5A028AB0-8423-4495-88FD-28E0318289AE";
	_serviceId = @"56702D92-69F9-4400-BEF8-D5A89FCFD31D";

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": { "message": "initalized" }
    }
  })JSON");

  [self.webview emit: "local-network" msg: msg];
}

- (void) startBluetooth {
  //
  // This ID is the same for all apps build with socket-sdk, this scopes all messages.
  // The channelUUID scopes the application and the developer can decide what to do after that.
  //
  auto serviceUUID = [CBUUID UUIDWithString: _serviceId]; // NSUUID
  _service = [[CBMutableService alloc] initWithType: serviceUUID primary: YES];
  auto channelUUID = [CBUUID UUIDWithString: _channelId];

  // NSData *channel = [NSData dataWithBytes: channelId.data() length: channelId.length()];
  // CBCharacteristicPropertyNotifiy

  _characteristic = [[CBMutableCharacteristic alloc]
    initWithType: channelUUID
      properties: CBCharacteristicPropertyNotify // (CBCharacteristicPropertyRead | CBCharacteristicPropertyWrite)
           value: nil
     permissions: CBAttributePermissionsReadable // (CBAttributePermissionsReadable | CBAttributePermissionsWriteable)
  ];

  _service.characteristics = @[_characteristic];

  [_peripheralManager addService: _service];

  //
  // Start advertising that we have a service with the SOCKET_CHANNEL UUID
  //
  [_peripheralManager startAdvertising: @{CBAdvertisementDataServiceUUIDsKey: @[_service.UUID]}];

  //
  // Start scanning for services that have the SOCKET_CHANNEL UUID
  //
  NSMutableArray* services = [NSMutableArray array];
  [services addObject: serviceUUID];

  [_centralManager
    scanForPeripheralsWithServices: services
    options: @{CBCentralManagerScanOptionAllowDuplicatesKey: @(NO)}    
  ];
}

/* - (void) peripheralManager: (CBPeripheralManager*)peripheral didReceiveReadRequest: (CBATTRequest*)request {
  NSLog(@"BBB peripheralManager:didReceiveReadRequest:");

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": { "message": "didReceiveReadRequest" }
    }
  })JSON");

  [self.webview emit: "local-network" msg: msg];

  if ([request.characteristic.UUID isEqual: _characteristic.UUID]) {
    if (request.offset > _characteristic.value.length) {
      [self.peripheralManager respondToRequest: request withResult: CBATTErrorInvalidOffset];
      return; // request was out of bounds
    }

    // TODO get the actual data
//     std::string s = "hello";
//         NSString* str = [NSString stringWithUTF8String: s.c_str()];
//             NSData* data = [str dataUsingEncoding: NSUTF8StringEncoding];
    request.value = data;
    [self.peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];
  }
}

- (void) peripheralManager: (CBPeripheralManager*)peripheral didReceiveWriteRequests: (NSArray<CBATTRequest*>*)requests {
  NSLog(@"BBB peripheralManager:didReceiveWriteRequests:");

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": { "message": "didReceiveWriteRequests" }
    }
  })JSON");

  [self.webview emit: "local-network" msg: msg];

  for (CBATTRequest* request in requests) {
    if (![request.characteristic.UUID isEqual: _characteristic.UUID]) {
      [self.peripheralManager respondToRequest: request withResult: CBATTErrorWriteNotPermitted];
      continue; // request was invalid
    }

    const void* rawData = [request.value bytes];
    char* src = (char*) rawData;

    // TODO return as a proper buffer
    auto msg = SSC::format(R"JSON({
      "value": {
        "data": { "message": "$S" }
      }
    })JSON", std::string(src));
    [self.webview emit: "local-network" msg: msg];
    [self.peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];  
  }
} */

- (void) localNetworkSend:(std::string)str uuid:(std::string)uuid {
  // auto serviceUUID = [CBUUID UUIDWithString: _serviceId];
  // auto channelUUID = [CBUUID UUIDWithString: _channelId];

  for (CBPeripheral* peripheral in _peripherals) {
    std::string id = [peripheral.identifier.UUIDString UTF8String];
    if ((uuid.size() > 0) && id != uuid) continue;

    // NSInteger amountToSend = self.dataToSend.length - self.sendDataIndex;
		// if (amountToSend > 128) amountToSend = 128;
    NSData *data = [NSData dataWithBytes: str.data() length: str.size()];
    auto didSend = [_peripheralManager updateValue: data
                                 forCharacteristic: _characteristic
                              onSubscribedCentrals: nil];

    if (!didSend) {
      NSLog(@"BBB did not send");
      return;
    }

    NSLog(@"BBB did send");
  }
}

- (void)centralManager:(CBCentralManager*)central didConnectPeripheral:(CBPeripheral*)peripheral {
  NSLog(@"BBB didConnectPeripheral");
  peripheral.delegate = self;
  [peripheral discoverServices: @[[CBUUID UUIDWithString:_serviceId]]];
}

- (void) centralManager: (CBCentralManager*)central didDiscoverPeripheral: (CBPeripheral*)peripheral advertisementData: (NSDictionary*)advertisementData RSSI: (NSNumber*)RSSI {
  std::string uuid = [peripheral.identifier.UUIDString UTF8String];
  std::string name = [peripheral.name UTF8String];

  if([_peripherals containsObject:peripheral]) return;

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": {
        "name": "$S",
        "uuid": "$S",
        "event": "peer-joined"
      }
    }
  })JSON", name, uuid);

  [self.webview emit: "local-network" msg: msg];

  [_peripherals addObject: peripheral];
  [central connectPeripheral: peripheral options: nil];
}

- (void) peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
  NSLog(@"BBB didDiscoverServices:");

  for (CBService *service in peripheral.services) {
    [peripheral discoverCharacteristics:@[[CBUUID UUIDWithString: _channelId]] forService: service];
  }
}

- (void) peripheral:(CBPeripheral*)peripheral didDiscoverCharacteristicsForService:(CBService*)service error:(NSError*)error {
  if (error) {
    NSLog(@"BBB ERROR didDiscoverCharacteristicsForService: %@", error);
    return;
  }

  NSLog(@"BBB didDiscoverCharacteristicsForService:");
  for (CBCharacteristic* characteristic in service.characteristics) {
    if ([characteristic.UUID isEqual: [CBUUID UUIDWithString: _channelId]]) {
      [peripheral setNotifyValue:YES forCharacteristic: characteristic];
    }
  }
}

- (void)peripheralManagerIsReadyToUpdateSubscribers:(CBPeripheralManager *)peripheral {
  auto msg = SSC::format(R"JSON({
    "value": {
      "data": {
        "message": "peripheralManagerIsReadyToUpdateSubscribers",
        "event": "status"
      }
    }
  })JSON");

  [self.webview emit: "local-network" msg: msg];
}

- (void) peripheral:(CBPeripheral*)peripheral didUpdateValueForCharacteristic:(CBCharacteristic*)characteristic error:(NSError*)error {
  if (error) {
    NSLog(@"ERROR didUpdateValueForCharacteristic: %@", error);
    return;
  }

  std::string uuid = [peripheral.identifier.UUIDString UTF8String];
  std::string name = [peripheral.name UTF8String];
  const void* rawData = [characteristic.value bytes];
  char* src = (char*) rawData;

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": {
        "name": "$S",
        "uuid": "$S",
        "data": "$S",
        "event": "peer-message"
      }
    }
  })JSON", name, uuid, std::string(src));

  NSLog(@"BBB didUpdateValueForCharacteristic: %s", src);

  [self.webview emit: "local-network" msg: msg];
}

- (void)peripheral:(CBPeripheral*)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic*)characteristic error:(NSError*)error {
  if (![characteristic.UUID isEqual:[CBUUID UUIDWithString: _channelId]]) {
    return;
  }

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": { 
        "message": "didUpdateNotificationStateForCharacteristic",
        "event": "status"
      }
    }
  })JSON");

  [self.webview emit: "local-network" msg: msg];
}

- (void) centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(nullable NSError *)error {
  NSLog(@"BBB didFailToConnectPeripheral: %@", [peripheral.identifier UUIDString]);
}

- (void) centralManager:(CBCentralManager*)central didDisconnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error {
  std::string uuid = [peripheral.identifier.UUIDString UTF8String];
  std::string name = [peripheral.name UTF8String];

  auto msg = SSC::format(R"JSON({
    "value": {
      "data": {
        "name": "$S",
        "uuid": "$S",
        "event": "peer-parted"
      }
    }
  })JSON", name, uuid);

  [self.webview emit: "local-network" msg: msg];

  [_peripherals removeObject: peripheral];
}
@end

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WV*)webView
  decidePolicyForNavigationAction: (WKNavigationAction*)navigationAction
  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@implementation NavigationDelegate
- (void) webView: (WKWebView*) webView
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  std::string base = webView.URL.absoluteString.UTF8String;
  std::string request = navigationAction.request.URL.absoluteString.UTF8String;

  if (request.find("file://") == 0) {
    decisionHandler(WKNavigationActionPolicyAllow);
  } else {
    decisionHandler(WKNavigationActionPolicyCancel);
  }
}
@end

@interface IPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (strong, nonatomic) WV* webview;
- (void)webView: (WV*)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
- (void)webView: (WV*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
@end

@implementation IPCSchemeHandler
- (void)webView: (WV*)wv stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask {}
- (void)webView: (WV*)wv startURLSchemeTask:(id<WKURLSchemeTask>)task {
  SSC::Core* core = wv.core;
  auto url = std::string(task.request.URL.absoluteString.UTF8String);

  SSC::Parse cmd(url);

  if (cmd.name == "post") {
    uint64_t postId = std::stoll(cmd.get("id"));
    auto post = core->getPost(postId);
    NSMutableDictionary* httpHeaders;

    if (post.length > 0) {
      httpHeaders[@"Content-Length"] = @(post.length);
      auto lines = SSC::split(post.headers, ',');

      for (auto& line : lines) {
        auto pair = SSC::split(line, ':');
        NSString* key = [NSString stringWithUTF8String: pair[0].c_str()];
        NSString* value = [NSString stringWithUTF8String: pair[1].c_str()];
        httpHeaders[key] = value;
      }
    }

    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: httpHeaders
    ];

    [task didReceiveResponse: httpResponse];

    if (post.length > 0) {
      NSString* str = [NSString stringWithUTF8String: post.body];
      NSData* data = [str dataUsingEncoding: NSUTF8StringEncoding];
      [task didReceiveData: data];
    }

    [task didFinish];

    core->removePost(postId);
    return;
  }

  core->putTask(cmd.get("seq"), task);

  char* body = NULL;

  // if there is a body on the reuqest, pass it into the method router.
	auto rawBody = task.request.HTTPBody;
  if (rawBody) {
    const void* data = [rawBody bytes];
    body = (char*)data;
  }

  [delegate route: url buf: body];
}
@end
