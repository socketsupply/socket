#import <Webkit/Webkit.h>
#include <_types/_uint64_t.h>
#import <UIKit/UIKit.h>
#import <Network/Network.h>
#include "common.hh"
#include "apple.hh"
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "include/uv.h"
#include "include/udx.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WKWebView*)webView
    decidePolicyForNavigationAction: (WKNavigationAction*)navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) SocketCore* core;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
- (void) route: (std::string)route;
@end

@interface IPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (strong, nonatomic) AppDelegate* delegate;
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
@end

@implementation IPCSchemeHandler
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask {}
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)task {
  AppDelegate* delegate = self.delegate;
  SocketCore* core = delegate.core;
  
  auto url = std::string(task.request.URL.absoluteString.UTF8String);

  SSC::Parse cmd(url);

  if (cmd.name == "post") {
    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: nil
    ];
      
    [task didReceiveResponse: httpResponse];

    uint64_t postId = std::stoll(cmd.get("id"));

    auto post = [core getPost: postId];

    if (post != nullptr) {
      [task didReceiveData: post];
    }

    [task didFinish];

    [core removePost: postId];
    return;
  }

  tasks.insert_or_assign(cmd.get("seq"), task);
  [delegate route: url];
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

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) route: (std::string)msg {
  using namespace SSC;
  if (msg.find("ipc://") != 0) return;

  Parse cmd(msg);
  auto seq = cmd.get("seq");
  uint64_t clientId = 0;

  if (cmd.get("fsRmDir").size() != 0) {
    [self.core fsRmDir: seq
             path: cmd.get("path")];
  }
    
  if (cmd.get("fsOpen").size() != 0) {
    [self.core fsOpen: seq
              id: std::stoll(cmd.get("id"))
            path: cmd.get("path")
           flags: std::stoi(cmd.get("flags"))];
  }

  if (cmd.get("fsClose").size() != 0) {
    [self.core fsClose: seq
               id: std::stoll(cmd.get("id"))];
  }

  if (cmd.get("fsRead").size() != 0) {
    [self.core fsRead: seq
              id: std::stoll(cmd.get("id"))
             len: std::stoi(cmd.get("len"))
          offset: std::stoi(cmd.get("offset"))];
  }

  if (cmd.get("fsWrite").size() != 0) {
    [self.core fsWrite: seq
               id: std::stoll(cmd.get("id"))
             data: cmd.get("data")
           offset: std::stoll(cmd.get("offset"))];
  }

  if (cmd.get("fsStat").size() != 0) {
    [self.core fsStat: seq
            path: cmd.get("path")];
  }

  if (cmd.get("fsUnlink").size() != 0) {
    [self.core fsUnlink: seq
              path: cmd.get("path")];
  }

  if (cmd.get("fsRename").size() != 0) {
    [self.core fsRename: seq
             pathA: cmd.get("oldPath")
             pathB: cmd.get("newPath")];
  }

  if (cmd.get("fsCopyFile").size() != 0) {
    [self.core fsCopyFile: seq
           pathA: cmd.get("src")
           pathB: cmd.get("dest")
           flags: std::stoi(cmd.get("flags"))];
  }

  if (cmd.get("fsMkDir").size() != 0) {
    [self.core fsMkDir: seq
              path: cmd.get("path")
              mode: std::stoi(cmd.get("mode"))];
  }

  if (cmd.get("fsReadDir").size() != 0) {
    [self.core fsReadDir: seq
              path: cmd.get("path")];
  }

  if (cmd.get("clientId").size() != 0) {
    try {
      clientId = std::stoll(cmd.get("clientId"));
    } catch (...) {
      [self.core resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid clientid" }
      })JSON")];
      return;
    }
  }

  if (cmd.name == "external") {
    NSString *url = [NSString stringWithUTF8String:SSC::decodeURIComponent(cmd.get("value")).c_str()];
    [[UIApplication sharedApplication] openURL: [NSURL URLWithString:url] options: @{} completionHandler: nil];
    return;
  }

  if (cmd.name == "ip") {
    auto seq = cmd.get("seq");

    if (clientId == 0) {
      [self.core reject: seq message: SSC::format(R"JSON({
        "err": {
          "message": "no clientid"
        }
      })JSON")];
    }

    Client* client = clients[clientId];

    if (client == nullptr) {
      [self.core resolve: seq message: SSC::format(R"JSON({
        "err": {
          "message": "not connected"
        }
      })JSON")];
    }

    PeerInfo info;
    info.init(client->tcp);

    auto message = SSC::format(
      R"JSON({
        "data": {
          "ip": "$S",
          "family": "$S",
          "port": "$i"
        }
      })JSON",
      clientId,
      info.ip,
      info.family,
      info.port
    );

    [self resolve: seq message: message];
    return;
  }

  if (cmd.name == "getNetworkInterfaces") {
    [self.core resolve: seq message: [self.core getNetworkInterfaces]
    ];
    return;
  }

  if (cmd.name == "readStop") {
    [self.core readStop: seq clientId: clientId];
    return;
  }

  if (cmd.name == "shutdown") {
    [self.core shutdown: seq clientId: clientId];
    return;
  }

  if (cmd.name == "readStop") {
    [self.core readStop: seq clientId: clientId];
    return;
  }

  if (cmd.name == "sendBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    [self.core sendBufferSize: seq clientId: clientId size: size];
    return;
  }

  if (cmd.name == "recvBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    [self.core recvBufferSize: seq clientId: clientId size: size];
    return;
  }

  if (cmd.name == "close") {
    [self.core close: seq clientId: clientId];
    return;
  }

  if (cmd.name == "udpSend") {
    int offset = 0;
    int len = 0;
    int port = 0;

    try {
      offset = std::stoi(cmd.get("offset"));
    } catch (...) {
      [self.core resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid offset" }
      })JSON")];
    }

    try {
      len = std::stoi(cmd.get("len"));
    } catch (...) {
      [self.core resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid length" }
      })JSON")];
    }

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      [self.core resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid port" }
      })JSON")];
    }

    [self.core udpSend: seq
         clientId: clientId
          message: cmd.get("data")
           offset: offset
              len: len
             port: port
               ip: (const char*) cmd.get("ip").c_str()
    ];
    return;
  }

  if (cmd.name == "tpcSend") {
    [self.core tcpSend: clientId
          message: cmd.get("message")
    ];
    return;
  }

  if (cmd.name == "tpcConnect") {
    int port = 0;

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      [self.core resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid port" }
      })JSON")];
    }

    [self.core tcpConnect: seq
            clientId: clientId
                port: port
                  ip: cmd.get("ip")
    ];
    return;
  }

  if (cmd.name == "tpcSetKeepAlive") {
    [self.core tcpSetKeepAlive: seq
                 clientId: clientId
                  timeout: std::stoi(cmd.get("timeout"))
    ];
    return;
  }

  if (cmd.name == "tpcSetTimeout") {
    [self.core tcpSetTimeout: seq
               clientId: clientId
                timeout: std::stoi(cmd.get("timeout"))
    ];
    return;
  }

  if (cmd.name == "tpcBind") {
    auto serverId = cmd.get("serverId");
    auto ip = cmd.get("ip");
    auto port = cmd.get("port");
    auto seq = cmd.get("seq");

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    if (ip == "error") {
      [self.core resolve: cmd.get("seq")
            message: SSC::format(R"({
              "err": { "message": "offline" }
            })")
      ];
      return;
    }

    if (port.size() == 0) {
      [self.core reject: cmd.get("seq")
           message: SSC::format(R"({
             "err": { "message": "port required" }
           })")
      ];
      return;
    }

    [self.core tcpBind: seq
         serverId: std::stoll(serverId)
               ip: ip
             port: std::stoi(port)
    ];

    return;
  }

  if (cmd.name == "dnsLookup") {
    [self.core dnsLookup: seq
           hostname: cmd.get("hostname")
    ];
    return;
  }

  NSLog(@"%s", msg.c_str());
}

- (void) applicationDidEnterBackground {
  [self.core.webview evaluateJavaScript: @"window.blur()" completionHandler:nil];
}

- (void) applicationWillEnterForeground {
  [self.core.webview evaluateJavaScript: @"window.focus()" completionHandler:nil];
}

//
// Next two methods handle creating the renderer and receiving/routing messages
//
- (void) userContentController :(WKUserContentController *) userContentController
  didReceiveScriptMessage :(WKScriptMessage *) scriptMessage {
    id body = [scriptMessage body];
    if (![body isKindOfClass:[NSString class]]) {
      return;
    }

    [self route: [body UTF8String]];
}

- (BOOL) application: (UIApplication *)app openURL: (NSURL*)url options: (NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
  auto str = std::string(url.absoluteString.UTF8String);

  // TODO can this be escaped or is the url encoded property already?
  [self.core emit: "protocol" message: SSC::format(R"JSON({
    "url": "$S",
  })JSON", str)];

  return YES;
}

- (BOOL) application :(UIApplication *) application
  didFinishLaunchingWithOptions :(NSDictionary *) launchOptions {
    using namespace SSC;

    auto core = self.core = [SocketCore new];
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

    core.content = [config userContentController];

    [core.content addScriptMessageHandler:self name: @"webview"];
    [core.content addUserScript: initScript];

    core.webview = [[WKWebView alloc] initWithFrame: appFrame configuration: config];
    core.webview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    [core.webview.configuration.preferences setValue: @YES forKey: @"allowFileAccessFromFileURLs"];
    [core.webview.configuration.preferences setValue: @YES forKey: @"javaScriptEnabled"];

    self.navDelegate = [[NavigationDelegate alloc] init];
    [core.webview setNavigationDelegate: self.navDelegate];

    [viewController.view addSubview: core.webview];

    NSString* allowed = [[NSBundle mainBundle] resourcePath];
    NSString* url = [allowed stringByAppendingPathComponent:@"ui/index.html"];

    [core.webview loadFileURL: [NSURL fileURLWithPath: url]
      allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
    ];

    [self.window makeKeyAndVisible];
    [core initNetworkStatusObserver];

    return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
