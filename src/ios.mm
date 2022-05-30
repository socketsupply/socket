#import <Webkit/Webkit.h>
#include <_types/_uint64_t.h>
#import <UIKit/UIKit.h>
#import <Network/Network.h>
#import "common.hh"
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "apple.hh"
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

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler, SocketIO>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@end

@interface IPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (strong, nonatomic) AppDelegate* delegate;
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
@end

std::map<std::string, id<WKURLSchemeTask>> tasks;
std::map<uint64_t, NSData*> postRequests;

@implementation IPCSchemeHandler
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask {}
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)task {
  auto url = std::string(task.request.URL.absoluteString.UTF8String);

  SSC::Parse cmd(url);

  if (cmd.name === "post") {
    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: nil
    ];

    [task didReceiveResponse: httpResponse];

    uint64_t id = std::stoll(cmd.get("id"));
    if (postRequests.find(id) == postRequests.end()) {
      return;
    }

    [task didReceiveData: postRequests[id]];
    [task didFinish];

    postRequests.erase(id);
    return;
  }

  tasks[cmd.get("seq")] = task;
  [self.delegate route: url];
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
- (void) emit: (std::string)event message: (std::string)message {
  NSString* script = [NSString stringWithUTF8String: SSC::emitToRenderProcess(event, SSC::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}

- (void) send: (std::string)params buf: (char*)buf {
  uint64_t id = SSC::rand64();
  std::string sid = std::to_string(id);

  std::string js(
    "const xhx = new XMLHttpRequest();"
    "xhr.open('ipc://post?id=" + sid + "');"
    "xhr.onload = e => {"
    "  const o = new URLSearchParams('" + params + "');"
    "  const detail = {"
    "    data: xhr.response," +
    "    params: Object.fromEntries(o)"
    "  };"
    "  window._ipc.emit('data', detail);"
    "}"
  );

  NSString* str = [NSString stringWithUTF8String:buf];
  NSData* data = [str dataUsingEncoding: NSUTF8StringEncoding];
  postRequests[id] = data;
  
  NSString* script = [NSString stringWithUTF8String: js.c_str()];
  [self.webview evaluateJavaScript: initRequest completionHandler: nil];
}

- (void) resolve: (std::string)seq message: (std::string)message {
  if (tasks.find(seq) != tasks.end()) {
    auto task = tasks[seq];

    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: nil
    ];

    [task didReceiveResponse: httpResponse];

    NSString* str = [NSString stringWithUTF8String:message.c_str()];
    NSData* data = [str dataUsingEncoding: NSUTF8StringEncoding];

    [task didReceiveData: data];
    [task didFinish];

    tasks.erase(seq);
    return;
  }

  NSString* script = [NSString stringWithUTF8String: SSC::resolveToRenderProcess(seq, "0", SSC::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}

- (void) reject: (std::string)seq message: (std::string)message {
  NSString* script = [NSString stringWithUTF8String: SSC::resolveToRenderProcess(seq, "1", SSC::encodeURIComponent(message)).c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}

- (void) applicationDidEnterBackground {
  [self.webview evaluateJavaScript: @"window.blur()" completionHandler:nil];
}

- (void) applicationWillEnterForeground {
  [self.webview evaluateJavaScript: @"window.focus()" completionHandler:nil];
}

- (void) route: (std::string)msg {
  using namespace SSC;
  if (msg.find("ipc://") != 0) return;

  Parse cmd(msg);
  auto seq = cmd.get("seq");
  uint64_t clientId = 0;

  if (cmd.get("fsOpen").size() != 0) {
    [self fsOpen: seq
              id: std::stoll(cmd.get("id"))
            path: cmd.get("path")
           flags: std::stoi(cmd.get("flags"))];
  }

  if (cmd.get("fsClose").size() != 0) {
    [self fsClose: seq
               id: std::stoll(cmd.get("id"))];
  }

  if (cmd.get("fsRead").size() != 0) {
    [self fsRead: seq
              id: std::stoll(cmd.get("id"))
             len: std::stoi(cmd.get("len"))
          offset: std::stoi(cmd.get("offset"))];
  }

  if (cmd.get("fsWrite").size() != 0) {
    [self fsWrite: seq
               id: std::stoll(cmd.get("id"))
             data: cmd.get("data")
           offset: std::stoll(cmd.get("offset"))];
  }

  if (cmd.get("fsStat").size() != 0) {
    [self fsStat: seq
            path: cmd.get("path")];
  }

  if (cmd.get("fsUnlink").size() != 0) {
    [self fsUnlink: seq
              path: cmd.get("path")];
  }

  if (cmd.get("fsRename").size() != 0) {
    [self fsRename: seq
             pathA: cmd.get("oldPath")
             pathB: cmd.get("newPath")];
  }

  if (cmd.get("fsCopyFile").size() != 0) {
    [self fsCopyFile: seq
           pathA: cmd.get("src")
           pathB: cmd.get("dest")
           flags: std::stoi(cmd.get("flags"))];
  }

  if (cmd.get("fsRmDir").size() != 0) {
    [self fsRmDir: seq
              path: cmd.get("path")];
  }

  if (cmd.get("fsMkDir").size() != 0) {
    [self fsMkDir: seq
              path: cmd.get("path")
              mode: std::stoi(cmd.get("mode"))];
  }

  if (cmd.get("fsReadDir").size() != 0) {
    [self fsReadDir: seq
              path: cmd.get("path")];
  }

  if (cmd.get("clientId").size() != 0) {
    try {
      clientId = std::stoll(cmd.get("clientId"));
    } catch (...) {
      [self resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid clientid" }
      })JSON")];
      return;
    }
  }

  NSLog(@"COMMAND %s", msg.c_str());

  if (cmd.name == "external") {
    NSString *url = [NSString stringWithUTF8String:SSC::decodeURIComponent(cmd.get("value")).c_str()];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
    return;
  }

  if (cmd.name == "ip") {
    auto seq = cmd.get("seq");

    if (clientId == 0) {
      [self reject: seq message: SSC::format(R"JSON({
        "err": {
          "message": "no clientid"
        }
      })JSON")];
    }

    Client* client = clients[clientId];

    if (client == nullptr) {
      [self resolve: seq message: SSC::format(R"JSON({
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
    [self resolve: seq message: [self getNetworkInterfaces]
    ];
    return;
  }

  if (cmd.name == "readStop") {
    [self readStop: seq clientId: clientId];
    return;
  }

  if (cmd.name == "shutdown") {
    [self shutdown: seq clientId: clientId];
    return;
  }

  if (cmd.name == "readStop") {
    [self readStop: seq clientId: clientId];
    return;
  }

  if (cmd.name == "sendBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    [self sendBufferSize: seq clientId: clientId size: size];
    return;
  }

  if (cmd.name == "recvBufferSize") {
    int size;
    try {
      size = std::stoi(cmd.get("size"));
    } catch (...) {
      size = 0;
    }

    [self recvBufferSize: seq clientId: clientId size: size];
    return;
  }

  if (cmd.name == "close") {
    [self close: seq clientId: clientId];
    return;
  }

  if (cmd.name == "udpSend") {
    int offset = 0;
    int len = 0;
    int port = 0;

    try {
      offset = std::stoi(cmd.get("offset"));
    } catch (...) {
      [self resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid offset" }
      })JSON")];
    }

    try {
      len = std::stoi(cmd.get("len"));
    } catch (...) {
      [self resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid length" }
      })JSON")];
    }

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      [self resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid port" }
      })JSON")];
    }

    [self udpSend: seq
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
    [self tcpSend: clientId
          message: cmd.get("message")
    ];
    return;
  }

  if (cmd.name == "tpcConnect") {
    int port = 0;

    try {
      port = std::stoi(cmd.get("port"));
    } catch (...) {
      [self resolve: seq message: SSC::format(R"JSON({
        "err": { "message": "invalid port" }
      })JSON")];
    }

    [self tcpConnect: seq
            clientId: clientId
                port: port
                  ip: cmd.get("ip")
    ];
    return;
  }

  if (cmd.name == "tpcSetKeepAlive") {
    [self tcpSetKeepAlive: seq
                 clientId: clientId
                  timeout: std::stoi(cmd.get("timeout"))
    ];
    return;
  }

  if (cmd.name == "tpcSetTimeout") {
    [self tcpSetTimeout: seq
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
      [self resolve: cmd.get("seq")
            message: SSC::format(R"({
              "err": { "message": "offline" }
            })")
      ];
      return;
    }

    if (port.size() == 0) {
      [self reject: cmd.get("seq")
           message: SSC::format(R"({
             "err": { "message": "port required" }
           })")
      ];
      return;
    }

    [self tcpBind: seq
         serverId: std::stoll(serverId)
               ip: ip
             port: std::stoi(port)
    ];

    return;
  }

  if (cmd.name == "dnsLookup") {
    [self dnsLookup: seq
           hostname: cmd.get("hostname")
    ];
    return;
  }

  if (cmd.name == "udxSocketInit") {
    [self udxSocketInit: seq
                  udxId: std::stoll(cmd.get("udxId"))
               socketId: std::stoll(cmd.get("socketId"))
    ];
    return;
  }

  if (cmd.name == "udxInit") {
    [self udxInit: seq
            udxId: std::stoll(cmd.get("udxId"))
    ];
    return;
  }

  if (cmd.name == "udxSocketBind") {
    [self udxSocketBind: seq
               socketId: std::stoll(cmd.get("socketId"))
                   port: std::stoi(cmd.get("port"))
                     ip: cmd.get("ip")
    ];
    return;
  }

  if (cmd.name == "udxSocketClose") {
    [self udxSocketClose: seq
                socketId: std::stoll(cmd.get("socketId"))
    ];
    return;
  }

  if (cmd.name == "udxSocketSetTTL") {
    [self udxSocketSetTTL: seq
                 socketId: std::stoll(cmd.get("socketId"))
                      ttl: std::stoi(cmd.get("ttl"))
    ];
    return;
  }

  if (cmd.name == "udxSocketRecvBufferSize") {
    [self udxSocketRecvBufferSize: seq
                         socketId: std::stoll(cmd.get("socketId"))
                             size: std::stoi(cmd.get("size"))
    ];
    return;
  }

  if (cmd.name == "udxSocketSendBufferSize") {
    [self udxSocketSendBufferSize: seq
                         socketId: std::stoll(cmd.get("socketId"))
                             size: std::stoi(cmd.get("size"))
    ];
    return;
  }

  if (cmd.name == "udxStreamInit") {
    [self udxStreamInit: seq
                  udxId: std::stoll(cmd.get("udxId"))
               streamId: std::stoll(cmd.get("streamId"))
                     id: std::stoi(cmd.get("id"))
    ];
    return;
  }

  if (cmd.name == "udxStreamDestroy") {
    [self udxStreamDestroy: seq
                   streamId: std::stoll(cmd.get("streamId"))
    ];
    return;
  }

  if (cmd.name == "udxStreamSetMode") {
    [self udxStreamSetMode: seq
                  streamId: std::stoll(cmd.get("streamId"))
                      mode: std::stoi(cmd.get("mode"))
    ];
    return;
  }

  NSLog(@"%s", msg.c_str());
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
  [self emit: "protocol" message: SSC::format(R"JSON({
    "url": "$S",
  })JSON", str)];

  return YES;
}

- (BOOL) application :(UIApplication *) application
  didFinishLaunchingWithOptions :(NSDictionary *) launchOptions {
    using namespace SSC;

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

    self.navDelegate = [[NavigationDelegate alloc] init];
    [self.webview setNavigationDelegate: self.navDelegate];

    [viewController.view addSubview: self.webview];

    NSString* allowed = [[NSBundle mainBundle] resourcePath];
    NSString* url = [allowed stringByAppendingPathComponent:@"ui/index.html"];

    [self.webview loadFileURL: [NSURL fileURLWithPath: url]
      allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
    ];

    // NSString *protocol = @"socket-sdk-loader://";
    // [self.webview loadRequest: [protocol stringByAppendingString: url]];

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

