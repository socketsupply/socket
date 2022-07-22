#include "core.hh"
#import <CoreBluetooth/CoreBluetooth.h>
#import <UserNotifications/UserNotifications.h>

//
// Mixed-into ios.mm and mac.hh by #include. This file
// expects BridgedWebView to be defined before it's included.
// All IO is routed though these common interfaces.
//
@class Bridge;

dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("ssc.queue", qos);

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webview: (BridgedWebView*)webview
  decidePolicyForNavigationAction: (WKNavigationAction*)navigationAction
  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface BluetoothDelegate : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate>
@property (strong, nonatomic) Bridge* bridge;
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) NSMutableDictionary* services;
@property (strong, nonatomic) NSMutableDictionary* characteristics;
@property (strong, nonatomic) NSMutableDictionary* serviceMap;
- (void) publishCharacteristic: (std::string)seq buf: (char*)buf len: (int)len sid: (std::string)sid cid: (std::string)cid;
- (void) subscribeCharacteristic: (std::string)seq sid: (std::string)sid cid: (std::string)cid;
- (void) startService: (std::string)seq sid: (std::string)sid;
- (void) startAdvertising;
- (void) startScanning;
- (void) initBluetooth;
@end

@interface IPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (strong, nonatomic) Bridge* bridge;
- (void)webView: (BridgedWebView*)webview startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
- (void)webView: (BridgedWebView*)webview stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
@end

@interface Bridge : NSObject
@property (strong, nonatomic) BluetoothDelegate* bluetooth;
@property (strong, nonatomic) BridgedWebView* webview;
@property (nonatomic) SSC::Core* core;
- (bool) route: (std::string)msg buf: (char*)buf;
- (void) emit: (std::string)name msg: (std::string)msg;
- (void) send: (std::string)seq msg: (std::string)msg post: (SSC::Post)post;
- (void) setBluetooth: (BluetoothDelegate*)bd;
- (void) setWebview: (BridgedWebView*)bv;
- (void) setCore: (SSC::Core*)core;
@end

@implementation BluetoothDelegate
// - (void)disconnect {
// }

// - (void)updateRssi {
//  NSLog(@"CoreBluetooth: updateRssi");
// }

- (void) stopAdvertising {
  NSLog(@"CoreBluetooth: stopAdvertising");

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
      [self startAdvertising];
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

  auto msg = SSC::format(R"MSG({
    "data": {
      "event: "state",
      "state": "$S"
    }
  })MSG", message, state);

  [self.bridge emit: "bluetooth" msg: msg];
}

- (void) peripheralManagerDidStartAdvertising: (CBPeripheralManager*)peripheral error: (NSError*)error {
  if (error) {
    NSLog(@"CoreBluetooth: %@", error);
    auto desc = std::string([error.debugDescription UTF8String]);
    std::replace(desc.begin(), desc.end(), '"', '\''); // Secure

    auto msg = SSC::format(R"MSG({
      "err": {
        "event": "peripheralManagerDidStartAdvertising",
        "message": "$S"
      }
    })MSG", desc);

    [self.bridge emit: "bluetooth" msg: msg];
    return;
  }

  NSLog(@"CoreBluetooth didStartAdvertising: %@", _serviceMap);
}

- (void) peripheralManager: (CBPeripheralManager*)peripheralManager central: (CBCentral*)central didSubscribeToCharacteristic: (CBCharacteristic*)characteristic {
  NSLog(@"CoreBluetooth: didSubscribeToCharacteristic");
}

- (void) centralManagerDidUpdateState: (CBCentralManager*)central {
  NSLog(@"CoreBluetooth: centralManagerDidUpdateState");
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
  NSArray* keys = [_serviceMap allKeys];
  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  [_peripheralManager stopAdvertising];
  [_peripheralManager startAdvertising: @{CBAdvertisementDataServiceUUIDsKey: [uuids copy]}];
}

- (void) startScanning {
  NSArray* keys = [_serviceMap allKeys];
  if ([keys count] == 0) {
    NSLog(@"No keys to scan for");
    return;
  }

  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  NSLog(@"CoreBluetooth: startScanning %@", uuids);

  [_centralManager
    scanForPeripheralsWithServices: [uuids copy]
    options: @{CBCentralManagerScanOptionAllowDuplicatesKey: @(YES)}
  ];
}

-(void) subscribeCharacteristic: (std::string)seq sid: (std::string)sid cid: (std::string)cid {
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

  auto msg = SSC::format(R"MSG({
    "data": {
      "serviceId": "$S",
      "characteristicId": "$S",
      "message": "Started"
    }
  })MSG", sid, cid);

  [self.bridge send: seq msg: msg post: SSC::Post{}];
}

- (void) publishCharacteristic: (std::string)seq buf: (char*)buf len: (int)len sid: (std::string)sid cid: (std::string)cid {
  NSString* ssid = [NSString stringWithUTF8String: sid.c_str()];
  NSString* scid = [NSString stringWithUTF8String: cid.c_str()];

  auto sUUID = [CBUUID UUIDWithString: ssid];
  auto cUUID = [CBUUID UUIDWithString: scid];

  if (!_serviceMap[ssid]) {
    auto msg = SSC::format(R"MSG({ "err": { "message": "invalid serviceId" } })MSG");
    [self.bridge send: seq msg: msg post: SSC::Post{}];
    return;
  }

  if (![_serviceMap[ssid] containsObject: cUUID]) {
    auto msg = SSC::format(R"MSG({ "err": { "message": "invalid characteristicId" } })MSG");
    [self.bridge send: seq msg: msg post: SSC::Post{}];
    return;
  }

  [self startScanning]; // noop if already scanning.
  [self startAdvertising]; // noop if already advertising.

  if (len == 0) {
    NSLog(@"CoreBluetooth: characteristic added");
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
    NSLog(@"CoreBluetooth: did not write");
    return;
  }

  NSLog(@"CoreBluetooth: did write '%@' %@", data, characteristic);
}

- (void) startService: (std::string)seq sid: (std::string)sid { // start advertising and scanning for a new service
  NSString* ssid = [NSString stringWithUTF8String: sid.c_str()];

  auto sUUID = [CBUUID UUIDWithString: ssid];
  CBMutableService* service = _services[ssid];

  if (!service) {
    service = [[CBMutableService alloc] initWithType: sUUID primary: YES];

    service.characteristics = [_characteristics[ssid] copy];

    [_services setValue: service forKey: ssid];
    [_peripheralManager addService: service];
  }

  [self startAdvertising];
  [self startScanning];

  auto msg = SSC::format(R"MSG({
    "data": {
      "serviceId": "$S",
      "message": "Started"
    }
  })MSG", sid);

  [self.bridge send:seq msg: msg post:SSC::Post{}];
}

- (void) peripheralManager: (CBPeripheralManager*)peripheral didReceiveReadRequest: (CBATTRequest*)request {
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
    NSLog(@"CoreBluetooth: No local characteristic found for request");
    return;
  }

  request.value = ch.value;
  [_peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];

  [self startScanning];
  [self startAdvertising];
}

- (void) centralManager: (CBCentralManager*)central didDiscoverPeripheral: (CBPeripheral*)peripheral advertisementData: (NSDictionary*)advertisementData RSSI: (NSNumber*)RSSI {
  if (peripheral.identifier == nil || peripheral.name == nil) {
    [self.peripherals addObject: peripheral];

    NSTimeInterval _scanTimeout = 0.5;
    [NSTimer timerWithTimeInterval: _scanTimeout repeats: NO block:^(NSTimer* timer) {
      NSLog(@"CoreBluetooth: reconnecting");
      [self->_centralManager connectPeripheral: peripheral options: nil];
    }];
    return;
  }

  std::string uuid = std::string([peripheral.identifier.UUIDString UTF8String]);
  std::string name = std::string([peripheral.name UTF8String]);

  if (uuid.size() == 0 || name.size() == 0) {
    NSLog(@"device has no meta information");
    return;
  }

  auto isConnected = peripheral.state != CBPeripheralStateDisconnected;
  auto isKnown = [_peripherals containsObject: peripheral];

  if (isKnown && isConnected) {
    return;
  }

  auto msg = SSC::format(R"MSG({
    "data" : {
      "event": "peerDiscovered",
      "name": "$S",
      "uuid": "$S"
    }
  })MSG", name, uuid);

  [self.bridge emit: "bluetooth" msg: msg];

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

- (void) centralManager: (CBCentralManager*)central didConnectPeripheral: (CBPeripheral*)peripheral {
  NSLog(@"CoreBluetooth: didConnectPeripheral");
  peripheral.delegate = self;
  // [peripheral setNotifyValue: YES forCharacteristic: _characteristic];

  NSArray* keys = [_serviceMap allKeys];
  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  [peripheral discoverServices: uuids];
}

- (void) peripheral: (CBPeripheral*)peripheral didDiscoverServices: (NSError*)error {
  if (error) {
    NSLog(@"CoreBluetooth: peripheral:didDiscoverService:error: %@", error);
    return;
  }

  for (CBService *service in peripheral.services) {
    NSString* key = service.UUID.UUIDString;
    NSArray* uuids = [_serviceMap[key] allObjects];

    NSLog(@"CoreBluetooth: peripheral:didDiscoverServices withChracteristics: %@", uuids);
    [peripheral discoverCharacteristics: [uuids copy] forService: service];
  }
}

- (void) peripheral: (CBPeripheral*)peripheral didDiscoverCharacteristicsForService: (CBService*)service error: (NSError*)error {
  if (error) {
    NSLog(@"CoreBluetooth: peripheral:didDiscoverCharacteristicsForService:error: %@", error);
    return;
  }

  NSString* key = service.UUID.UUIDString;
  NSArray* uuids = [[_serviceMap[key] allObjects] copy];

  for (CBCharacteristic* characteristic in service.characteristics) { // loop over discovered
    for (CBUUID* cUUID in uuids) { // for each known
      if ([characteristic.UUID isEqual: cUUID]) { // if its a match
        [peripheral setNotifyValue: YES forCharacteristic: characteristic];
        [peripheral readValueForCharacteristic: characteristic];
      }
    }
  }
}

- (void) peripheralManagerIsReadyToUpdateSubscribers: (CBPeripheralManager*)peripheral {
  /* auto msg = SSC::format(R"MSG({
    "data": {
      "source": "bluetooth",
      "message": "peripheralManagerIsReadyToUpdateSubscribers",
      "event": "status"
    }
  })MSG");

  [self.bridge emit: "bluetooth" msg: msg]; */
}

- (void) peripheral: (CBPeripheral*)peripheral didUpdateValueForCharacteristic: (CBCharacteristic*)characteristic error:(NSError*)error {
  if (error) {
    NSLog(@"ERROR didUpdateValueForCharacteristic: %@", error);
    return;
  }

  if (!characteristic.value || characteristic.value.length == 0) return;

  std::string uuid = "";
  std::string name = "";

  if (peripheral.identifier != nil) {
    uuid = [peripheral.identifier.UUIDString UTF8String];
  }

  if (peripheral.name != nil) {
    name = [peripheral.name UTF8String];
    std::replace(name.begin(), name.end(), '\n', ' '); // Secure
  }

  std::string characteristicId = [characteristic.UUID.UUIDString UTF8String];
  std::string sid = "";

  for (NSString* ssid in [_serviceMap allKeys]) {
    if ([_serviceMap[ssid] containsObject: characteristic.UUID]) {
      sid = [ssid UTF8String];
    }
  }

  SSC::Post post;
  post.body = (char*)characteristic.value.bytes;
  post.length = (int)characteristic.value.length;

  post.headers = SSC::format(R"MSG(
    content-type: application/octet-stream
    content-length: $i
  )MSG", post.length);

  std::string seq = "-1";

  std::string msg = SSC::format(R"MSG({
    "data": {
      "event": "data",
      "source": "bluetooth",
      "characteristicId": "$S",
      "serviceId": "$S",
      "name": "$S",
      "uuid": "$S"
    }
  })MSG", characteristicId, sid, name, uuid);

  [self.bridge send: seq msg: msg post: post];
}

- (void) peripheral: (CBPeripheral*)peripheral didUpdateNotificationStateForCharacteristic: (CBCharacteristic*)characteristic error: (NSError*)error {
}

- (void) centralManager: (CBCentralManager*)central didFailToConnectPeripheral: (CBPeripheral*)peripheral error: (NSError*)error {
  // if (error != nil) {
  //  NSLog(@"CoreBluetooth: failed to connect %@", error.debugDescription);
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

- (void) centralManager: (CBCentralManager*)central didDisconnectPeripheral: (CBPeripheral*)peripheral error: (NSError*)error {
  [_centralManager connectPeripheral: peripheral options: nil];

  if (error != nil) {
    NSLog(@"CoreBluetooth: device did disconnect %@", error.debugDescription);
    return;
  }
}
@end

@implementation NavigationDelegate
- (void) webview: (BridgedWebView*) webview
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  std::string base = webview.URL.absoluteString.UTF8String;
  std::string request = navigationAction.request.URL.absoluteString.UTF8String;

  if (request.find("file://") == 0 && request.find("http://localhost") == 0) {
    decisionHandler(WKNavigationActionPolicyCancel);
  } else {
    decisionHandler(WKNavigationActionPolicyAllow);
  }
}
@end

@implementation Bridge
- (void) setBluetooth: (BluetoothDelegate*)bd {
  _bluetooth = bd;
  [_bluetooth initBluetooth];
  _bluetooth.bridge = self;
}

- (void) setWebview: (BridgedWebView*)wv {
  _webview = wv;
}

- (void) setCore: (SSC::Core*)core; {
  _core = core;
}

- (void) emit: (std::string)name msg: (std::string)msg {
  msg = SSC::emitToRenderProcess(name, SSC::encodeURIComponent(msg));
  NSString* script = [NSString stringWithUTF8String: msg.c_str()];
  [self.webview evaluateJavaScript: script completionHandler: nil];
}

- (void) send: (std::string)seq msg: (std::string)msg post: (SSC::Post)post {
  //
  // - If there is no sequence and there is a buffer, the source is a stream and it should
  // invoke the client to ask for it via an XHR, this will be intercepted by the scheme handler.
  // - On the next turn, it will have a sequence and a task that will respond to the XHR which
  // already has the meta data from the original request.
  //
  if (seq == "-1" && post.length > 0) {
    auto src = self.core->createPost(msg, post);
    NSString* script = [NSString stringWithUTF8String: src.c_str()];
    [self.webview evaluateJavaScript: script completionHandler: nil];
    return;
  }

  if ((seq != "-1") && (post.length > 0) && self.core->hasTask(seq)) {
    auto task = self.core->getTask(seq);

    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: nil
    ];

    [task didReceiveResponse: httpResponse];

    NSData* data;

    // if post has a length, use the post's body as the response...
    if (post.length > 0) {
      data = [NSData dataWithBytes: post.body length: post.length];
    } else {
      NSString* str = [NSString stringWithUTF8String: msg.c_str()];
      data = [str dataUsingEncoding: NSUTF8StringEncoding];
    }

    [task didReceiveData: data];
    [task didFinish];

    self.core->removeTask(seq);
    return;
  }

  if (seq != "-1") { // this had a sequence, we need to try to resolve it.
    msg = SSC::resolveToRenderProcess(seq, "0", SSC::encodeURIComponent(msg));
  }

  NSString* script = [NSString stringWithUTF8String: msg.c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}

-(void)userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler {
  completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}

// returns true if routable (regardless of success)
- (bool) route: (std::string)msg buf: (char*)buf {
  using namespace SSC;

  if (msg.find("ipc://") != 0) return false;

  Parse cmd(msg);
  auto seq = cmd.get("seq");
  uint64_t clientId = 0;

  /// ipc bluetooth-start
  /// @param serviceId String
  ///
  if (cmd.name == "bluetooth-start") {
    [self.bluetooth startService: seq sid: cmd.get("serviceId")];
    return true;
  }

  if (cmd.name == "bluetooth-subscribe") {
    auto cid = cmd.get("characteristicId");
    auto sid = cmd.get("serviceId");
    auto seq = cmd.get("seq");

    [self.bluetooth subscribeCharacteristic: seq sid: sid cid: cid];
    return true;
  }

  if (cmd.name == "bluetooth-publish") {
    auto sid = cmd.get("serviceId");
    auto cid = cmd.get("characteristicId");
    auto seq = cmd.get("seq");

    if (sid.size() != 36) {
      auto msg = SSC::format(R"MSG({ "err": { "message": "invalid serviceId" } })MSG");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    if (sid.size() != 36) {
      auto msg = SSC::format(R"MSG({ "err": { "message": "invalid characteristicId" } })MSG");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    char* value;
    int len;

    if (buf != nullptr) {
      len = std::stoi(cmd.get("length"));
      value = buf;
    } else {
      value = cmd.get("value").data();
      len = (int) cmd.get("value").size();
    }

    [self.bluetooth publishCharacteristic: seq buf: value len: len sid: sid cid: cid];
    return true;
  }

  if (cmd.name == "notify") {
    UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
    content.body = [NSString stringWithUTF8String: cmd.get("body").c_str()];
    content.title = [NSString stringWithUTF8String: cmd.get("title").c_str()];
    content.sound = [UNNotificationSound defaultSound];

    UNTimeIntervalNotificationTrigger* trigger = [
      UNTimeIntervalNotificationTrigger triggerWithTimeInterval: 1.0f repeats: NO
    ];

    UNNotificationRequest *request = [UNNotificationRequest
      requestWithIdentifier: @"LocalNotification" content: content trigger: trigger
    ];

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    // center.delegate = self;

    [center requestAuthorizationWithOptions: (UNAuthorizationOptionSound | UNAuthorizationOptionAlert | UNAuthorizationOptionBadge) completionHandler:^(BOOL granted, NSError* error) {
      if(!error) {
        [center addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
          if (error) NSLog(@"Unable to create notification: %@", error.debugDescription);
        }];
      }
    }];
    return true;
  }

  if (cmd.name == "log") {
    NSLog(@"%s", cmd.get("value").c_str());
    return true;
  }

  if (cmd.get("fsConstants").size() != 0) {
    dispatch_async(queue, ^{
      auto constants = self.core->getFSConstants();
      [self send: seq msg: constants post: Post{}];
    });
    return true;
  }

  if (cmd.get("fsRmDir").size() != 0) {
    auto path = cmd.get("path");

    dispatch_async(queue, ^{
      self.core->fsRmDir(seq, path, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsOpen").size() != 0) {
    auto cid = std::stoull(cmd.get("id"));
    auto path = cmd.get("path");
    auto flags = std::stoi(cmd.get("flags"));
    auto mode = std::stoi(cmd.get("mode"));

    dispatch_async(queue, ^{
      self.core->fsOpen(seq, cid, path, flags, mode, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsClose").size() != 0) {
    auto id = std::stoull(cmd.get("id"));

    dispatch_async(queue, ^{
      self.core->fsClose(seq, id, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsRead").size() != 0) {
    auto id = std::stoull(cmd.get("id"));
    auto len = std::stoi(cmd.get("len"));
    auto offset = std::stoi(cmd.get("offset"));

    dispatch_async(queue, ^{
      self.core->fsRead(seq, id, len, offset, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsWrite").size() != 0) {
    auto id = std::stoull(cmd.get("id"));
    auto offset = std::stoull(cmd.get("offset"));

    dispatch_async(queue, ^{
      self.core->fsWrite(seq, id, buf, offset, [&](auto seq, auto msg, auto post) {

     dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsStat").size() != 0) {
    auto path = cmd.get("path");

    dispatch_async(queue, ^{
      self.core->fsStat(seq, path, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsFStat").size() != 0) {
    auto id = std::stoull(cmd.get("id"));

    dispatch_async(queue, ^{
      self.core->fsFStat(seq, id, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsUnlink").size() != 0) {
    auto path = cmd.get("path");

    dispatch_async(queue, ^{
      self.core->fsUnlink(seq, path, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsRename").size() != 0) {
    auto pathA = cmd.get("oldPath");
    auto pathB = cmd.get("newPath");

    dispatch_async(queue, ^{
      self.core->fsRename(seq, pathA, pathB, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsCopyFile").size() != 0) {
    auto pathA = cmd.get("src");
    auto pathB = cmd.get("dest");
    auto flags = std::stoi(cmd.get("flags"));

    dispatch_async(queue, ^{
      self.core->fsCopyFile(seq, pathA, pathB, flags, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsMkDir").size() != 0) {
    auto path = cmd.get("path");
    auto mode = std::stoi(cmd.get("mode"));

    dispatch_async(queue, ^{
      self.core->fsMkDir(seq, path, mode, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.get("fsReadDir").size() != 0) {
    auto path = cmd.get("path");

    dispatch_async(queue, ^{
      self.core->fsReadDir(seq, path, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  // TODO this is a generalization that doesnt work
  if (cmd.get("clientId").size() != 0) {
    try {
      clientId = std::stoull(cmd.get("clientId"));
    } catch (...) {
      auto msg = SSC::format(R"MSG({
        "err": { "message": "invalid clientid" }
      })MSG");
      [self send: seq msg: msg post: Post{}];
      return true;
    }
  }

  NSLog(@"COMMAND %s", msg.c_str());

  if (cmd.name == "external") {
    NSString *url = [NSString stringWithUTF8String:SSC::decodeURIComponent(cmd.get("value")).c_str()];
    #if MACOS == 1
      [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: url]];
    #else
      [[UIApplication sharedApplication] openURL: [NSURL URLWithString:url] options: @{} completionHandler: nil];
    #endif
    return true;
  }

  if (cmd.name == "ip") {
    auto seq = cmd.get("seq");

    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "not connected"
        }
      })MSG");
      [self send: seq msg: msg post: Post{}];
    }

    PeerInfo info;
    info.init(client->tcp);

    auto msg = SSC::format(
      R"MSG({
        "data": {
          "ip": "$S",
          "family": "$S",
          "port": "$i"
        }
      })MSG",
      clientId,
      info.ip,
      info.family,
      info.port
    );

    [self send: seq msg: msg post: Post{}];
    return true;
  }

  if (cmd.name == "getNetworkInterfaces") {
    auto msg = self.core->getNetworkInterfaces();
    [self send: seq msg: msg post: Post{} ];
    return true;
  }

  if (cmd.name == "getPlatformOS") {
    auto msg = SSC::format(R"JSON({
      "data": "$S"
    })JSON", SSC::platform.os);
    [self send: seq msg: msg post: Post{} ];
    return true;
  }

  if (cmd.name == "getPlatformType") {
    auto msg = SSC::format(R"JSON({
      "data": "$S"
    })JSON", SSC::platform.os);
    [self send: seq msg: msg post: Post{} ];
    return true;
  }

  if (cmd.name == "getPlatformArch") {
    auto msg = SSC::format(R"JSON({
      "data": "$S"
    })JSON", SSC::platform.arch);
    [self send: seq msg: msg post: Post{} ];
    return true;
  }

  if (cmd.name == "readStop") {
    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->readStop(seq, clientId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "shutdown") {
    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->shutdown(seq, clientId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "sendBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->sendBufferSize(seq, clientId, size, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "recvBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->recvBufferSize(seq, clientId, size, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "close") {
    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->close(seq, clientId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "udpSend") {
    int offset = 0;
    int len = 0;
    int port = 0;
    std::string err;

    try {
      offset = std::stoi(cmd.get("offset"));
    } catch (...) {
      err = "invalid offset";
    }

    try {
      len = std::stoi(cmd.get("len"));
    } catch (...) {
      err = "invalid length";
    }

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      err = "invalid port";
    }

    if (err.size() > 0) {
      auto msg = SSC::format(R"MSG({
        message": "$S" }
      })MSG", err);
      [self send: seq msg: err post: Post{}];
      return true;
    }

    auto ip = cmd.get("ip");
    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->udpSend(seq, clientId, buf, offset, len, port, (const char*) ip.c_str(), [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "tcpSend") {
    auto clientId = std::stoull(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->tcpSend(clientId, buf, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "tcpConnect") {
    int port = 0;

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      auto msg = SSC::format(R"MSG({
        "err": { "message": "invalid port" }
      })MSG");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    auto clientId = std::stoull(cmd.get("clientId"));
    auto ip = cmd.get("ip");

    dispatch_async(queue, ^{
      self.core->tcpConnect(seq, clientId, port, ip, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "tcpSetKeepAlive") {
    auto clientId = std::stoull(cmd.get("clientId"));
    auto timeout = std::stoi(cmd.get("timeout"));

    dispatch_async(queue, ^{
      self.core->tcpSetKeepAlive(seq, clientId, timeout, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "tcpSetTimeout") {
    auto clientId = std::stoull(cmd.get("clientId"));
    auto timeout = std::stoi(cmd.get("timeout"));

    dispatch_async(queue, ^{
      self.core->tcpSetTimeout(seq, clientId, timeout, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  if (cmd.name == "udpBind") {
    auto ip = cmd.get("ip");
    std::string err;
    int port;
    uint64_t serverId;

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    try {
      serverId = std::stoull(cmd.get("serverId"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'serverId' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'port' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    dispatch_async(queue, ^{
      self.core->udpBind(seq, serverId, ip, port, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });

    return true;
  }

  if (cmd.name == "udpReadStart") {
    uint64_t serverId;

    try {
      serverId = std::stoull(cmd.get("serverId"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'serverId' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    dispatch_async(queue, ^{
      self.core->udpReadStart(seq, serverId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });

    return true;
  }

  if (cmd.name == "tcpBind") {
    auto ip = cmd.get("ip");
    std::string err;

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    if (cmd.get("port").size() == 0) {
      auto msg = SSC::format(R"({
        "err": { "message": "port required" }
      })");

      [self send: seq msg: msg post: Post{}];
      return true;
		}

    auto serverId = std::stoull(cmd.get("serverId"));
    auto port = std::stoi(cmd.get("port"));

    dispatch_async(queue, ^{
      self.core->tcpBind(seq, serverId, ip, port, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });

    return true;
  }

  if (cmd.name == "dnsLookup") {
    auto hostname = cmd.get("hostname");

    dispatch_async(queue, ^{
      self.core->dnsLookup(seq, hostname, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return true;
  }

  NSLog(@"%s", msg.c_str());
  return false;
}
@end

@implementation IPCSchemeHandler
- (void)webView: (BridgedWebView*)webview stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask {}
- (void)webView: (BridgedWebView*)webview startURLSchemeTask:(id<WKURLSchemeTask>)task {
  auto url = std::string(task.request.URL.absoluteString.UTF8String);

  SSC::Parse cmd(url);

  if (cmd.name == "post") {
    uint64_t postId = std::stoull(cmd.get("id"));
    auto post = self.bridge.core->getPost(postId);
    NSMutableDictionary* httpHeaders = [NSMutableDictionary dictionary];

    if (post.length > 0) {
      httpHeaders[@"content-length"] = [@(post.length) stringValue];
      httpHeaders[@"access-control-allow-origin"] = @"*";
      auto lines = SSC::split(SSC::trim(post.headers), '\n');

      for (auto& line : lines) {
        auto pair = SSC::split(SSC::trim(line), ':');
        NSString* key = [NSString stringWithUTF8String: SSC::trim(pair[0]).c_str()];
        NSString* value = [NSString stringWithUTF8String: SSC::trim(pair[1]).c_str()];
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

    self.bridge.core->removePost(postId);
    return;
  }

  self.bridge.core->putTask(cmd.get("seq"), task);
  char* body = NULL;

  // if there is a body on the reuqest, pass it into the method router.
	auto rawBody = task.request.HTTPBody;

  if (rawBody) {
    const void* data = [rawBody bytes];
    body = (char*)data;
  }

  [self.bridge route: url buf: body];
}
@end
