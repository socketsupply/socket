#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import "common.hh"
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

@class AppDelegate;

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WKWebView*) webView decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@implementation NavigationDelegate
- (void) webView: (WKWebView*) webView
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  std::string base = webView.URL.absoluteString.UTF8String;
  std::string request = navigationAction.request.URL.absoluteString.UTF8String;

    NSLog(@"OPKIT navigation %s", request.c_str());
  if (request.find("file://") == 0) {
    decisionHandler(WKNavigationActionPolicyAllow);
  } else {
    decisionHandler(WKNavigationActionPolicyCancel);
  }
}
@end

@interface Server : NSObject
@property CFSocketRef listener;
@property NSMutableArray* connections;
@property NSString* port;
@property WKWebView* webview;
- (Server*) listen: (WKWebView*) webview port: (NSNumber*) port hostname: (NSString*)hostname;
- (void) emitToRenderProcess: (std::string)event message: (std::string)message;
@end

@interface Client : NSObject
@property CFSocketRef socket;
@property Server* server;
@property NSMutableArray* connections;
@property CFRunLoopSourceRef rls;
@property NSData* _config;
+ (Client*) connect: (CFSocketNativeHandle)handle server: (Server*)server;
- (Client*) init: (CFSocketNativeHandle)handle server: (Server*)server;
- (void) receive:(CFDataRef)data;
@end

@implementation Client
+ (Client*) connect: (CFSocketNativeHandle)handle server: (Server*)server {
  Client* client = [Client alloc];

  if ([client init: handle server: server] != nil) {
    return client;
  }

  return nil;
}
- (Client*) init: (CFSocketNativeHandle)handle server: (Server*)server {
  self.server = server;
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
      case kCFSocketDataCallBack:
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

    @synchronized(self.server) {
      [self.server.connections removeObject:self];
      NSLog(@"Client disconnected");
    }
  }

  NSString* msg = [[NSString alloc] initWithData:(__bridge NSData*)data encoding:NSUTF8StringEncoding];
  NSLog(@"message: %@", msg);
}
@end

@implementation Server
- (void) emitToRenderProcess: (std::string)event message: (std::string)message {
  NSString* script = [NSString stringWithUTF8String: Opkit::emitToRenderProcess(event, Opkit::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}
- (Server*) listen: (WKWebView*) webview port: (NSNumber*) port hostname: (NSString*)hostname {
  self.connections = [NSMutableArray arrayWithCapacity:10];

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
         Client* client = [Client connect:*handle server:server];

         if (client != nil) {
           @synchronized(server) {
             [server.connections addObject:client];
             [server emitToRenderProcess: "server" message: "{\"data\":\"connected\"}"];
           }
         }
         break;
       }
       default:
         NSLog(@"unexpected socket event");
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
  CFSocketError e = CFSocketSetAddress(self.listener, dataAddr);
  CFRelease(dataAddr);

  if (e) {
    NSLog(@"bind error %d", (int) e);
  }

  CFRunLoopSourceRef rls = CFSocketCreateRunLoopSource(nil, self.listener, 0);
  CFRunLoopAddSource(CFRunLoopGetMain(), rls, kCFRunLoopCommonModes);
  CFRelease(rls);

  return self;
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@property Server* server;
@property Client* client;
@property NSString* hostname;
@property NSNumber* port;
@end

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) eval: (std::string)message {
  auto script = [NSString stringWithUTF8String:message.c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}
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

      if (cmd.name == "getIP") {
        auto hostname = std::string([self.hostname UTF8String]);
        auto port = std::to_string([self.port intValue]);

        [self eval: Opkit::resolveToRenderProcess(cmd.get("seq"), "0",
          Opkit::encodeURIComponent(
            "{\"data\":{\"ip\":\"" + hostname + "\",\"port\":\"" + port + "\"}}"
          ))
        ];
        return;
      }

      if (cmd.name == "netBind") {
        if (self.server) {
          [self eval: resolveToRenderProcess(cmd.get("seq"), "0", Opkit::encodeURIComponent("{\"data\":\"EXISTS\"}"))];
          return;
        }

        auto hostname = cmd.get("hostname");
        auto port = cmd.get("port");

        if (hostname.size() == 0) {
          hostname = std::string([[self getIPAddress] UTF8String]);
        }

        NSLog(@"ip -> %s", hostname.c_str());

        if (port.size() == 0) {
          [self eval: Opkit::resolveToRenderProcess(
            cmd.get("seq"),
            "0",
            Opkit::encodeURIComponent("{\"err\":\"Port required\"}")
          )];
        }

        self.server = [Server alloc];
        self.port = [NSNumber numberWithInt:std::stoi(port)];
        self.hostname = [NSString stringWithUTF8String: hostname.c_str()];

        [self.server listen: self.webview port: self.port hostname: self.hostname];

        return;
      }
    }

    NSLog(@"OPKIT: console '%s'", msg.c_str());
}

- (BOOL) application :(UIApplication *) application
  didFinishLaunchingWithOptions :(NSDictionary *) launchOptions {
    using namespace Opkit;

    NSLog(@"OPKIT: finished loading");

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

