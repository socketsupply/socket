#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Network/Network.h>
#import "common.hh"
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

_Static_assert((RAND_MAX & (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

uint64_t rand64(void) {
  uint64_t r = 0;
  for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
    r <<= RAND_MAX_WIDTH;
    r ^= (unsigned) rand();
  }
  return r;
}

@interface Bridge : NSObject
@property WKWebView* webview;
- (void) emit: (std::string)event message: (std::string)message;
- (void) resolve: (std::string)seq message: (std::string)message;
- (void) reject: (std::string)seq message: (std::string)message;
@end

@interface Server : NSObject
@property CFSocketRef listener;
@property Bridge* bridge;
@property NSMutableArray* connections;
@property NSString* port;
- (Server*) listen: (WKWebView*) webview port: (NSNumber*) port hostname: (NSString*)hostname;
@end

@interface Client : NSObject
@property CFSocketRef socket;
@property Server* server;
@property Bridge* bridge;
@property CFRunLoopSourceRef rls;
@property NSData* _config;
@property CFDataRef dataAddr;
@property uint64_t clientid;
+ (Client*) adopt: (uint64_t) clientid handle: (CFSocketNativeHandle)handle server: (Server*)server;
- (Client*) connect: (uint64_t) clientid port: (NSNumber*)port address: (NSString*)address;
- (Client*) init: (uint64_t) clientid handle: (CFSocketNativeHandle)handle server: (Server*)server;
- (void) receive:(CFDataRef)data;
- (void) send:(NSString*)message;
@end

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WKWebView*) webView decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@property Server* server;
@property Client* client;
@property Bridge* bridge;
@property NSMutableArray* connections;
@property NSString* hostname;
@property NSNumber* port;
@end

@implementation Bridge
- (void) emit: (std::string)event message: (std::string)message {
  NSString* script = [NSString stringWithUTF8String: Opkit::emitToRenderProcess(event, Opkit::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}
- (void) resolve: (std::string)seq message: (std::string)message {
  NSString* script = [NSString stringWithUTF8String: Opkit::resolveToRenderProcess(seq, "0", Opkit::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}
- (void) reject: (std::string)seq message: (std::string)message {
  NSString* script = [NSString stringWithUTF8String: Opkit::resolveToRenderProcess(seq, "1", Opkit::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}
@end

@implementation Client
- (Client*) connect: (uint64_t) clientid port: (NSNumber*)port address: (NSString*)address {
  self.clientid = clientid;

  CFSocketContext info;
  memset(&info, 0, sizeof(info));
  info.info = (void*)CFBridgingRetain(self);

  auto onConnect = +[] (
   CFSocketRef s,
   CFSocketCallBackType callbackType,
   CFDataRef address,
   const void *data,
   void *info
   ) {
    Client* client = (__bridge Client*)info;

    switch (callbackType) {
      case kCFSocketDataCallBack:
        NSLog(@"RECEIVED DATA %@", data);
        [client receive:(CFDataRef) data];
        break;

      default:
        NSLog(@"unexpected socket event");
        break;
    }
  };

  self.socket = CFSocketCreate(nil, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketReadCallBack | kCFSocketAcceptCallBack | kCFSocketDataCallBack, onConnect, &info);

  int t = 1;
  setsockopt(CFSocketGetNative(self.socket), SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t));

  struct sockaddr_in addr;
  addr.sin_addr.s_addr = inet_addr([address cStringUsingEncoding:NSASCIIStringEncoding]);
  addr.sin_family = AF_INET;
  addr.sin_port = htons([port intValue]);

  self.dataAddr = CFDataCreate(nil, (const uint8_t*)&addr, sizeof(addr));

  CFSocketConnectToAddress(self.socket, self.dataAddr, 10);
  self.rls = CFSocketCreateRunLoopSource(nil, self.socket, 0);
  CFRunLoopAddSource(CFRunLoopGetMain(), self.rls, kCFRunLoopCommonModes);

  return self;
}
+ (Client*) adopt: (uint64_t) clientid handle: (CFSocketNativeHandle)handle server: (Server*)server {
  Client* client = [Client alloc];

  if ([client init: clientid handle: handle server: server] != nil) {
    return client;
  }

  return nil;
}
- (Client*) init: (uint64_t) clientid handle: (CFSocketNativeHandle)handle server: (Server*)server {
  self.server = server;
  self.clientid = clientid;
  CFSocketContext info;
  memset(&info, 0, sizeof(info));
  info.info = (void*)CFBridgingRetain(self);

  auto onConnect = +[](
   CFSocketRef s,
   CFSocketCallBackType callbackType,
   CFDataRef address,
   const void *data,
   void *info) {
    Client* client = (__bridge Client*)info;

    switch (callbackType) {
      case kCFSocketAcceptCallBack:
        NSLog(@"ACCEPT");
        break;
      case kCFSocketDataCallBack:
        NSLog(@"RECEIVE");
        [client receive:(CFDataRef) data];
        break;

      default:
        NSLog(@"unexpected socket event");
        break;
    }
  };

  self.socket = CFSocketCreateWithNative(nil, handle, kCFSocketDataCallBack, onConnect, &info);
  self.rls = CFSocketCreateRunLoopSource(nil, self.socket, 0);

  CFRunLoopAddSource(CFRunLoopGetMain(), self.rls, kCFRunLoopCommonModes);
  return self;
}

- (void) receive:(CFDataRef)data {
  if (CFDataGetLength(data) == 0) {
    CFSocketInvalidate(self.socket);
    self.socket = nil;

    if (self.server) {
      @synchronized(self.server) {
        [self.server.connections removeObject:self];
        // TODO add address info to JSON
        [self.bridge emit: "socket" message: Opkit::format(R"({
          "type": "disconnect",
          "clientid": "$S",
          "data": {}
        })", (int) self.clientid)];
      }
    } else {
      @synchronized(self) {
        // [self.connections removeObject:self];
        // TODO add address info to JSON
      }
    }

    return;
  }

  NSString* str = [[NSString alloc] initWithData:(__bridge NSData*)data encoding:NSUTF8StringEncoding];
  std::string message([str UTF8String]);
  std::string clientid = std::to_string((int) self.clientid);

  message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());

  [self.bridge emit: "socket" message: Opkit::format(R"({
    "type": "receive",
    "clientid": "$S",
    "data": "$S"
  })", clientid, message)];
}

- (void) send:(NSString*)message {
  NSData *data = [message dataUsingEncoding:NSUTF8StringEncoding];
  CFSocketError err = CFSocketSendData(self.socket, self.dataAddr, (CFDataRef) data, 128);

  if (err < 0) {
    NSLog(@"unable to write to socket");

    [self.bridge emit: "socket" message: Opkit::format(R"({
      "type": "error",
      "clientid": "",
      "data": "Unable to write to socket"
    })")];
  }
}
@end

@implementation Server
- (Server*) listen: (WKWebView*) webview port: (NSNumber*) port hostname: (NSString*)hostname {
  self.connections = [NSMutableArray arrayWithCapacity:128];

  CFSocketContext info;
  memset(&info, 0, sizeof(info));
  info.info = (void*)CFBridgingRetain(self);

  auto onConnect = +[] (
   CFSocketRef s,
   CFSocketCallBackType callbackType,
   CFDataRef address,
   const void *data,
   void *info
   ) {
     Server* server = (__bridge Server*)info;

     switch (callbackType) {
       case kCFSocketAcceptCallBack: {
         CFSocketNativeHandle* handle = (CFSocketNativeHandle*) data;
         uint64_t clientId = rand64();
         Client* client = [Client adopt:clientId handle: *handle server:server];

         if (client != nil) {
           @synchronized(server) {
             [server.connections addObject:client];
             // TODO add connecton info
             [server.bridge emit: "server" message: Opkit::format(R"({
               "type": "connected",
               "data": {}
             })")];
           }
         }
         break;
       }
       default:
         [server.bridge emit: "server" message: Opkit::format(R"({
           "type": "err",
           "data": "Unable to connect"
         })")];
         break;
     }
   };

  self.listener = CFSocketCreate(nil, PF_INET, SOCK_STREAM, IPPROTO_TCP, kCFSocketAcceptCallBack, onConnect, &info);

  int t = 1;
  setsockopt(CFSocketGetNative(self.listener), SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t));

  struct sockaddr_in addr;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons([self.port intValue]);

  CFDataRef dataAddr = CFDataCreate(nil, (const uint8_t*)&addr, sizeof(addr));
  CFSocketError err = CFSocketSetAddress(self.listener, dataAddr);
  CFRelease(dataAddr);

  if (err) {
    [self.bridge emit: "server" message: Opkit::format(R"({
      "type": "err",
      "data": "Unable to set socket address for listening"
    })")];
  }

  CFRunLoopSourceRef rls = CFSocketCreateRunLoopSource(nil, self.listener, 0);
  CFRunLoopAddSource(CFRunLoopGetMain(), rls, kCFRunLoopCommonModes);
  CFRelease(rls);

  return self;
}
@end

@implementation NavigationDelegate
- (void) webView: (WKWebView*) webView
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  std::string base = webView.URL.absoluteString.UTF8String;
  std::string request = navigationAction.request.URL.absoluteString.UTF8String;

  NSLog(@"naviate %s", request.c_str());

  if (request.find("file://") == 0) {
    decisionHandler(WKNavigationActionPolicyAllow);
  } else {
    decisionHandler(WKNavigationActionPolicyCancel);
  }
}
@end

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (NSString*) getIPAddress {
  NSString *address = @"error";
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  int success = getifaddrs(&interfaces);

  if (success == 0) {
    // Loop through linked list of interfaces
    temp_addr = interfaces;

    while (temp_addr != NULL) {
      if (temp_addr->ifa_addr->sa_family == AF_INET) {
        // Check if interface is en0 which is the wifi connection on the iPhone
        if ([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"]) {
          address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];
        }
      }
      temp_addr = temp_addr->ifa_next;
    }
  }
  freeifaddrs(interfaces);
  return address;
}

- (void) userContentController :(WKUserContentController *) userContentController
  didReceiveScriptMessage :(WKScriptMessage *) scriptMessage {
    using namespace Opkit;

    id body = [scriptMessage body];
    if (![body isKindOfClass:[NSString class]]) {
      return;
    }

    std::string msg = [body UTF8String];

    if (msg.find("ipc://") == 0) {
      Parse cmd(msg);

      if (cmd.name == "openExternal") {
        // NSString *url = [NSString stringWithUTF8String:cmd.value.c_str()];
        // [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
        return;
      }

      if (cmd.name == "getAddress") {
        auto hostname = std::string([self.hostname UTF8String]);
        auto port = std::to_string([self.port intValue]);

        [self.bridge resolve: cmd.get("seq") message: Opkit::format(R"({
          "data": {
            "ip": "$S",
            "port": "$S"
          }
        })", hostname, port)];

        return;
      }

      if (cmd.name == "send") {
        for (Client *client in self.connections) {
          uint64_t value;
          std::istringstream iss(cmd.get("clientid"));
          iss >> value;

          if (client.clientid == value) {
            [client send: [NSString stringWithUTF8String:cmd.get("message").c_str()]];

            [self.bridge resolve: cmd.get("seq") message: Opkit::format(R"({
              "data": "SENT"
            })")];
            return;
          }
        }

        [self.bridge resolve: cmd.get("seq") message: Opkit::format(R"({
          "err": "NOT SENT"
        })")];
        return;
      }

      if (cmd.name == "connect") {
        NSNumber* port = [NSNumber numberWithInt:std::stoi(cmd.get("port"))];
        NSString* address = [NSString stringWithUTF8String: cmd.get("address").c_str()];
        NSString* id = [NSString stringWithUTF8String: cmd.get("clientid").c_str()];

        Client* client = [Client alloc];
        client.bridge = self.bridge;

        [self.connections addObject:client];

        [client connect:strtoull([id UTF8String], NULL, 0) port:port address:address];

        [self.bridge resolve: cmd.get("seq") message: Opkit::format(R"({
          "data": {
            "port": "$S",
            "address": "$S"
          }
        })", cmd.get("port"), cmd.get("address"))];
        return;
      }

      if (cmd.name == "listen") {
        if (self.server) {
          [self.bridge resolve: cmd.get("seq") message: Opkit::format(R"({
            "data": "EXISTS"
          })")];
          return;
        }

        auto hostname = cmd.get("hostname");
        auto port = cmd.get("port");

        if (hostname.size() == 0) {
          hostname = std::string([[self getIPAddress] UTF8String]);
        }

        if (hostname == "error") {
          [self.bridge resolve: cmd.get("seq") message: Opkit::format(R"({
            "err": { "message": "offline" }
          })")];
          return;
        }

        if (port.size() == 0) {
          [self.bridge reject: cmd.get("seq") message: Opkit::format(R"({
            "err": { "message": "port required" }
          })")];
          return;
        }

        self.server = [Server alloc];
        self.server.bridge = self.bridge;
        self.port = [NSNumber numberWithInt:std::stoi(port)];
        self.hostname = [NSString stringWithUTF8String: hostname.c_str()];

        [self.server listen: self.webview port: self.port hostname: self.hostname];

        return;
      }
    }

    NSLog(@"%s", msg.c_str());
}

- (BOOL) application :(UIApplication *) application
  didFinishLaunchingWithOptions :(NSDictionary *) launchOptions {
    using namespace Opkit;

    auto appFrame = [[UIScreen mainScreen] bounds];

    self.window = [[UIWindow alloc] initWithFrame: appFrame];
    self.bridge = [Bridge alloc];
    self.connections = [NSMutableArray arrayWithCapacity:128];

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

    std::string preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
      "};\n"

      "console.log = (...args) => {\n"
      "  window.external.invoke(JSON.stringify(args));\n"
      "};\n"

      "console.error = console.warn = console.log;\n"
      "window.addEventListener('unhandledrejection', e => console.log(e.message));\n"
      "window.addEventListener('error', e => console.log(e.reason));\n"

      "" + createPreload(opts) + "\n"
    );

    // NSLog(@"%s", preload.c_str());

    WKUserScript* initScript = [[WKUserScript alloc]
      initWithSource: [NSString stringWithUTF8String: preload.c_str()]
      injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO];

    WKWebViewConfiguration *webviewConfig = [[WKWebViewConfiguration alloc] init];
    self.content = [webviewConfig userContentController];

    [self.content addScriptMessageHandler:self name: @"webview"];
    [self.content addUserScript: initScript];

    self.webview = [[WKWebView alloc] initWithFrame: appFrame configuration: webviewConfig];
    self.bridge.webview = self.webview;
    self.webview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    [self.webview.configuration.preferences setValue: @YES forKey: @"allowFileAccessFromFileURLs"];
    [self.webview.configuration.preferences setValue: @YES forKey: @"javaScriptEnabled"];

    self.navDelegate = [[NavigationDelegate alloc] init];
    [self.webview setNavigationDelegate: self.navDelegate];

    [viewController.view addSubview: self.webview];

    NSString* url = [[[NSBundle mainBundle] resourcePath]
      stringByAppendingPathComponent:@"ui/index.html"];

    NSString* allowed = [[NSBundle mainBundle] resourcePath];

    [self.webview
        loadFileURL: [NSURL fileURLWithPath:url]
        allowingReadAccessToURL: [NSURL fileURLWithPath:allowed]
    ];

    [self.window makeKeyAndVisible];
    return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}

