#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import "core.hh"

#include <_types/_uint64_t.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("ssc.queue", qos);

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WKWebView*)webView
    decidePolicyForNavigationAction: (WKNavigationAction*)navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;

@property nw_path_monitor_t monitor;
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
@property SSC::Core* core;
- (void) route: (std::string)msg buf: (char*)buf;
- (void) emit: (std::string)name msg: (std::string)msg;
- (void) send: (std::string)seq msg: (std::string)msg post: (SSC::Post)post;
@end


@interface BluetoothDelegate : NSObject<CBCentralManagerDelegate, CBPeripheralManagerDelegate>
@property (strong, nonatomic) AppDelegate* delegate;
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) CBMutableCharacteristic* channel;
- (void) initBluetooth: (std::string)channelUUID;
@end

@implementation BluetoothDelegate
AppDelegate* delegate;

- (void)disconnect {
   NSLog(@"disconnect");
   // throw new Error('disconnect is not supported on OS X!');
}

- (void)updateRssi {
  NSLog(@"updateRssi");
}

- (void)startAdvertisingIBeacon:(NSData *)data {
  NSLog(@"startAdvertisingIBeacon:%@", data);
}

- (void)stopAdvertising {
  NSLog(@"stopAdvertising");

  [self.peripheralManager stopAdvertising];
}

- (void)peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral {
    NSString *string = @"Unknown state";

    switch(peripheral.state) {
        case CBManagerStatePoweredOff:
            string = @"CoreBluetooth BLE hardware is powered off.";
            break;

        case CBManagerStatePoweredOn:
            string = @"CoreBluetooth BLE hardware is powered on and ready.";
            break;

        case CBManagerStateUnauthorized:
            string = @"CoreBluetooth BLE state is unauthorized.";
            break;

        case CBManagerStateUnknown:
            string = @"CoreBluetooth BLE state is unknown.";
            break;

        case CBManagerStateUnsupported:
            string = @"CoreBluetooth BLE hardware is unsupported on this platform.";
            break;

        default:
            break;
    }

    NSLog(@"%@", string);
}

- (void)peripheralManagerDidStartAdvertising:(CBPeripheralManager *)peripheral error:(nullable NSError *)error {
  NSLog(@"peripheralManagerDidStartAdvertising: %@", peripheral.description);
  if (error) {
      NSLog(@"Error advertising: %@", [error localizedDescription]);
  }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didSubscribeToCharacteristic:(CBMutableCharacteristic *)characteristic {
    NSLog(@"didSubscribeToCharacteristic");
}

- (void)peripheralManagerIsReadyToUpdateSubscribers:(CBPeripheralManager *)peripheral {
    NSLog(@"peripheralManagerIsReadyToUpdateSubscribers");
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didPublishL2CAPChannel:(CBL2CAPPSM)PSM error:(nullable NSError *)error {
    NSLog(@"didPublishL2CAPChannel");
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didUnpublishL2CAPChannel:(CBL2CAPPSM)PSM error:(nullable NSError *)error {
    NSLog(@"didUnpublishL2CAPChannel");
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didOpenL2CAPChannel:(nullable CBL2CAPChannel *)channel error:(nullable NSError *)error {
    NSLog(@"didOpenL2CAPChannel");
}

- (void)centralManagerDidUpdateState:(CBCentralManager *)central {}

- (void) initBluetooth: (std::string)channelUUID {
  self.centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
  self.peripheralManager = [[CBPeripheralManager alloc] initWithDelegate:self queue:nil options:nil];
  // 
  // This ID is the same for all apps build with socket-sdk, this scopes all messages.
  // The channelUUID scopes the application and the developer can decide what to do after that.
  // 
  std::string SOCKET_CHANNEL = "56702D92-69F9-4400-BEF8-D5A89FCFD31D";

  NSString* sid = [NSString stringWithUTF8String: SOCKET_CHANNEL.c_str()];
  auto serviceUUID = [CBUUID UUIDWithString: sid]; // NSUUID 
  auto service = [[CBMutableService alloc] initWithType: serviceUUID primary:YES];

  NSData *channel = [NSData dataWithBytes: channelUUID.data() length: channelUUID.length()];

  auto characteristic = [[CBMutableCharacteristic alloc]
    initWithType: serviceUUID
      properties: (CBCharacteristicPropertyRead | CBCharacteristicPropertyWrite)
           value: channel
     permissions: (CBAttributePermissionsReadable | CBAttributePermissionsWriteable)
  ];

  service.characteristics = @[characteristic];
  
  [self.peripheralManager addService: service];

  //
  // Start advertising that we have a service with the SOCKET_CHANNEL UUID
  //
  [self.peripheralManager startAdvertising: @{CBAdvertisementDataServiceUUIDsKey: @[service.UUID]}];

  //
  // Start scanning for services that have the SOCKET_CHANNEL UUID
  //
	NSMutableArray* services = [NSMutableArray array];
  [services addObject: serviceUUID];

  [self.centralManager
    scanForPeripheralsWithServices: services
    options: @{CBCentralManagerScanOptionAllowDuplicatesKey: @(YES)}    
  ];
}

- (void) peripheralManager: (CBPeripheralManager*)peripheral didReceiveReadRequest: (CBATTRequest*)request {
  if ([request.characteristic.UUID isEqual: self.channel.UUID]) {
    if (request.offset > self.channel.value.length) {
      [self.peripheralManager respondToRequest: request withResult: CBATTErrorInvalidOffset];
      return; // request was out of bounds
    }

    // TODO get the actual data
    std::string s = "hello";
    NSString* str = [NSString stringWithUTF8String: s.c_str()];
    NSData* data = [str dataUsingEncoding: NSUTF8StringEncoding];

    request.value = data;
    [self.peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];
  }
}

- (void) peripheralManager: (CBPeripheralManager*)peripheral didReceiveWriteRequests: (NSArray<CBATTRequest*>*)requests {
  for (CBATTRequest* request in requests) {
    if (![request.characteristic.UUID isEqual: self.channel.UUID]) {
      [self.peripheralManager respondToRequest: request withResult: CBATTErrorWriteNotPermitted];
      continue; // request was invalid
    }

    const void *_Nullable rawData = [request.value bytes];
    char *src = (char*) rawData;

    // TODO return as a proper buffer
    auto msg = SSC::format(R"JSON({
      "value": {
        "data": { "message": "$S" }
      }
    })JSON", std::string(src));

    [self.delegate emit: "bluetooth" msg: msg];
    [self.peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];
  }
}

- (void) centralManager: (CBCentralManager*)central didDiscoverPeripheral: (CBPeripheral*)peripheral advertisementData: (NSDictionary*)advertisementData RSSI: (NSNumber*)RSSI {
  NSLog(@"Discovered peripheral %@", peripheral.identifier.UUIDString);
  NSLog(@"Discovered peripheral %@", peripheral.name);

	[self.peripherals addObject: peripheral];
	[central connectPeripheral: peripheral options: nil];
}

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
@property (strong, nonatomic) AppDelegate* delegate;
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
@end

@implementation IPCSchemeHandler
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask {}
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)task {
  auto* delegate = self.delegate;
  SSC::Core* core = delegate.core;
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
    const void *_Nullable data = [rawBody bytes];
    body = (char*)data;
  }

  [delegate route: url buf: body];
}
@end

void uncaughtExceptionHandler (NSException *exception) {
  NSLog(@"%@", exception.name);
  NSLog(@"%@", exception.reason);
  NSLog(@"%@", exception.callStackSymbols);
}

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate

- (void) emit: (std::string)name msg: (std::string)msg {
  msg = SSC::emitToRenderProcess(name, SSC::encodeURIComponent(msg));
  NSString* script = [NSString stringWithUTF8String: msg.c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}

//
// This method is for when we want to send solicited or non-solicited data to the webview.
//
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

  if (self.core->hasTask(seq)) {
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

//
// This method determines what method to dispatch based on the uri.
//
- (void) route: (std::string)msg buf: (char*)buf {
  using namespace SSC;
  
  if (msg.find("ipc://") != 0) return;

  Parse cmd(msg);
  auto seq = cmd.get("seq");
  uint64_t clientId = 0;
  
  if (cmd.name == "external") {
    NSString *url = [NSString stringWithUTF8String:SSC::decodeURIComponent(cmd.get("value")).c_str()];
    [[UIApplication sharedApplication] openURL: [NSURL URLWithString:url] options: @{} completionHandler: nil];
    return;
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
  }

  if (cmd.get("fsOpen").size() != 0) {
    auto cid = std::stoll(cmd.get("id"));
    auto path = cmd.get("path");
    auto flags = std::stoi(cmd.get("flags"));

    dispatch_async(queue, ^{
      self.core->fsOpen(seq, cid, path, flags, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsClose").size() != 0) {
    auto id = std::stoll(cmd.get("id"));

    dispatch_async(queue, ^{
      self.core->fsClose(seq, id, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsRead").size() != 0) {
    auto id = std::stoll(cmd.get("id"));
    auto len = std::stoi(cmd.get("len"));
    auto offset = std::stoi(cmd.get("offset"));

    dispatch_async(queue, ^{
      self.core->fsRead(seq, id, len, offset, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsWrite").size() != 0) {
    auto id = std::stoll(cmd.get("id"));
    auto offset = std::stoll(cmd.get("offset"));

    dispatch_async(queue, ^{
      self.core->fsWrite(seq, id, buf, offset, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
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
  }

  if (cmd.get("clientId").size() != 0) {
    try {
      clientId = std::stoll(cmd.get("clientId"));
    } catch (...) {
      auto msg = SSC::format(R"JSON({
        "value": {
          "err": { "message": "invalid clientid" }
        }
      })JSON");
      [self send: seq msg: msg post: Post{}];
      return;
    }
  }

  if (cmd.name == "ip") {
    auto seq = cmd.get("seq");

    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"JSON({
        "value": {
          "err": {
            "message": "not connected"
          }
        }
      })JSON");
      [self send: seq msg: msg post: Post{}];
    }

    PeerInfo info;
    info.init(client->tcp);

    auto msg = SSC::format(
      R"JSON({
        "value": {
          "data": {
            "ip": "$S",
            "family": "$S",
            "port": "$i"
          }
        }
      })JSON",
      clientId,
      info.ip,
      info.family,
      info.port
    );

    [self send: seq msg: msg post: Post{}];
    return;
  }

  if (cmd.name == "getNetworkInterfaces") {
    auto msg = self.core->getNetworkInterfaces();
    [self send: seq msg: msg post: Post{} ];
    return;
  }

  if (cmd.name == "readStop") {
    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->readStop(seq, clientId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "shutdown") {
    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->shutdown(seq, clientId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "sendBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->sendBufferSize(seq, clientId, size, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "recvBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->recvBufferSize(seq, clientId, size, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "close") {
    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->close(seq, clientId, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
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
      auto msg = SSC::format(R"JSON({
        "err": { "message": "$S" }
      })JSON", err);
      [self send: seq msg: err post: Post{}];
      return;
    }
    
    auto ip = cmd.get("ip");
    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->udpSend(seq, clientId, buf, offset, len, port, (const char*) ip.c_str(), [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "tcpSend") {
    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      self.core->tcpSend(clientId, buf, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "tcpConnect") {
    int port = 0;

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      auto msg = SSC::format(R"JSON({
        "err": { "message": "invalid port" }
      })JSON");
      [self send: seq msg: msg post: Post{}];
    }

    auto clientId = std::stoll(cmd.get("clientId"));
    auto ip = cmd.get("ip");

    dispatch_async(queue, ^{
      self.core->tcpConnect(seq, clientId, port, ip, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "tcpSetKeepAlive") {
    auto clientId = std::stoll(cmd.get("clientId"));
    auto timeout = std::stoi(cmd.get("timeout"));

    dispatch_async(queue, ^{
      self.core->tcpSetKeepAlive(seq, clientId, timeout, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "tcpSetTimeout") {
    auto clientId = std::stoll(cmd.get("clientId"));
    auto timeout = std::stoi(cmd.get("timeout"));

    dispatch_async(queue, ^{
      self.core->tcpSetTimeout(seq, clientId, timeout, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
    return;
  }

  if (cmd.name == "tcpBind") {
    auto ip = cmd.get("ip");
    std::string err;

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    if (cmd.get("port").size() == 0) {
      auto msg = SSC::format(R"({
        "value": {
          "err": { "message": "port required" }
        }
      })");
      
      [self send: seq msg: msg post: Post{}];
      return;
    }

    auto serverId = std::stoll(cmd.get("serverId"));
    auto port = std::stoi(cmd.get("port"));

    dispatch_async(queue, ^{
      self.core->tcpBind(seq, serverId, ip, port, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });

    return;
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
    return;
  }

  NSLog(@"%s", msg.c_str());
}

- (void) applicationDidEnterBackground {
  [self.webview evaluateJavaScript: @"window.blur()" completionHandler:nil];
}

- (void) applicationWillEnterForeground {
  [self.webview evaluateJavaScript: @"window.focus()" completionHandler:nil];
}

- (void) initNetworkStatusObserver {
  dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_UTILITY,
    DISPATCH_QUEUE_PRIORITY_DEFAULT
  );

  self.monitorQueue = dispatch_queue_create("co.socketsupply.network-monitor", attrs);

  // self.monitor = nw_path_monitor_create_with_type(nw_interface_type_wifi);
  self.monitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(self.monitor, self.monitorQueue);
  nw_path_monitor_set_update_handler(self.monitor, ^(nw_path_t _Nonnull path) {
    nw_path_status_t status = nw_path_get_status(path);

    std::string name;
    std::string message;

    switch (status) {
      case nw_path_status_invalid: {
        name = "offline";
        message = "Network path is invalid";
        break;
      }
      case nw_path_status_satisfied: {
        name = "online";
        message = "Network is usable";
        break;
      }
      case nw_path_status_satisfiable: {
        name = "online";
        message = "Network may be usable";
        break;
      }
      case nw_path_status_unsatisfied: {
        name = "offline";
        message = "Network is not usable";
        break;
      }
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [self emit: name msg: SSC::format(R"JSON({
        "message": "$S"
      })JSON", message)];
    });
  });

  nw_path_monitor_start(self.monitor);
}

//
// Next two methods handle creating the renderer and receiving/routing messages
//
- (void) userContentController: (WKUserContentController*) userContentController didReceiveScriptMessage: (WKScriptMessage*)scriptMessage {
    id body = [scriptMessage body];

    if (![body isKindOfClass:[NSString class]]) {
      return;
    }

    [self route: [body UTF8String] buf: NULL];
}

- (BOOL) application: (UIApplication*)app openURL: (NSURL*)url options: (NSDictionary<UIApplicationOpenURLOptionsKey, id>*)options {
  auto str = std::string(url.absoluteString.UTF8String);

  // TODO can this be escaped or is the url encoded property already?
  [self emit: "protocol" msg: SSC::format(R"JSON({
    "url": "$S",
  })JSON", str)];

  return YES;
}

- (BOOL) application: (UIApplication*)application didFinishLaunchingWithOptions: (NSDictionary*)launchOptions {
  using namespace SSC;

  NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);

  self.core = new Core;

  auto appFrame = [[UIScreen mainScreen] bounds];

  self.window = [[UIWindow alloc] initWithFrame: appFrame];

  UIViewController *viewController = [[UIViewController alloc] init];
  viewController.view.frame = appFrame;
  self.window.rootViewController = viewController;

  auto appData = parseConfig(decodeURIComponent(_settings));

  std::stringstream env;

  for (auto const &envKey : split(appData["env"], ',')) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << std::string(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  env << std::string("width=" + std::to_string(appFrame.size.width) + "&");
  env << std::string("height=" + std::to_string(appFrame.size.height) + "&");

  WindowOptions opts {
    .debug = _debug,
    .executable = appData["executable"],
    .title = appData["title"],
    .version = appData["version"],
    .preload = gPreloadMobile,
    .env = env.str()
  };

  // Note: you won't see any logs in the preload script before the
  // Web Inspector is opened
  std::string  preload = Str(
    "window.addEventListener('unhandledrejection', e => console.log(e.message));\n"
    "window.addEventListener('error', e => console.log(e.reason));\n"

    "window.external = {\n"
    "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
    "};\n"

    "console.error = console.warn = console.log;\n"
    "" + createPreload(opts) + "\n"
    "//# sourceURL=preload.js"
  );

  WKUserScript* initScript = [[WKUserScript alloc]
    initWithSource: [NSString stringWithUTF8String: preload.c_str()]
    injectionTime: WKUserScriptInjectionTimeAtDocumentStart
    forMainFrameOnly: NO];

  WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

  auto handler = [IPCSchemeHandler new];
  handler.delegate = self;

  [config setURLSchemeHandler: handler forURLScheme:@"ipc"];

  self.content = [config userContentController];

  [self.content addScriptMessageHandler:self name: @"webview"];
  [self.content addUserScript: initScript];

  self.webview = [[WKWebView alloc] initWithFrame: appFrame configuration: config];
  self.webview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  [self.webview.configuration.preferences setValue: @YES forKey: @"allowFileAccessFromFileURLs"];
  [self.webview.configuration.preferences setValue: @YES forKey: @"javaScriptEnabled"];

  auto bluetoothDelegate = [BluetoothDelegate new];
  bluetoothDelegate.delegate = self;

  [bluetoothDelegate initBluetooth: appData["network_id"]];

  self.navDelegate = [[NavigationDelegate alloc] init];
  [self.webview setNavigationDelegate: self.navDelegate];

  [viewController.view addSubview: self.webview];

  NSString* allowed = [[NSBundle mainBundle] resourcePath];
  NSString* url = [allowed stringByAppendingPathComponent:@"ui/index.html"];

  [self.webview loadFileURL: [NSURL fileURLWithPath: url]
    allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
  ];

  [self.window makeKeyAndVisible];
  [self initNetworkStatusObserver];

  return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
