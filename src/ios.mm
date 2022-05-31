#import <Webkit/Webkit.h>
#include <_types/_uint64_t.h>
#import <UIKit/UIKit.h>

#import "common.hh"
#include "core.hh"

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
@property SSC::Core core;
- (void) route: (std::string)msg buf: (char*)buf;
- (void) send: (std::string)seq msg: (std::string)msg post: (SSC::Post)post;
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
  SSC::Core core = delegate.core;
  auto url = std::string(task.request.URL.absoluteString.UTF8String);

  SSC::Parse cmd(url);

  if (cmd.name == "post") {
    uint64_t postId = std::stoll(cmd.get("id"));
    auto post = core.getPost(postId);
    NSMutableDictionary* httpHeaders;

    if (post.length > 0) {
      httpHeaders[@"Content-Length"] = @(post.length);
      auto lines = SSC::splitByRegex(",", post.headers);

      for (auto& line : lines) {
        auto pair = SSC::splitByRegex(":", line);
        httpHeaders[pair[0]] = @(pair[1]);
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

    core.removePost(postId);
    return;
  }

  core.putTask(cmd.get("seq"), task);

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

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
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
    auto src = self.core.createPost(msg, post);
    NSString* script = [NSString stringWithUTF8String: src.c_str()];
    [self.webview evaluateJavaScript: script completionHandler: nil];
    return;
  }

  if (self.core.hasTask(seq)) {
    auto task = self.core.getTask(seq);

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

    self.core.removeTask(seq);
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
  Core core;
  
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
      core.fsRmDir(seq, path, [&](auto seq, auto msg, auto post) {
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
      core.fsOpen(seq, cid, path, flags, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsClose").size() != 0) {
    auto id = std::stoll(cmd.get("id"));

    dispatch_async(queue, ^{
      core.fsClose(seq, id, [&](auto seq, auto msg, auto post) {
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
      core.fsRead(seq, id, len, offset, [&](auto seq, auto msg, auto post) {
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
      core.fsWrite(seq, id, buf, offset, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsStat").size() != 0) {
    auto path = cmd.get("path");

    dispatch_async(queue, ^{
      core.fsStat(seq, path, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsUnlink").size() != 0) {
     auto path = cmd.get("path");

     dispatch_async(queue, ^{
       core.fsUnlink(seq, path, [&](auto seq, auto msg, auto post) {
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
      core.fsRename(seq, pathA, pathB, [&](auto seq, auto msg, auto post) {
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
      core.fsCopyFile(seq, pathA, pathB, flags, [&](auto seq, auto msg, auto post) {
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
      core.fsMkDir(seq, path, mode, [&](auto seq, auto msg, auto post) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self send: seq msg: msg post: post];
        });
      });
    });
  }

  if (cmd.get("fsReadDir").size() != 0) {
    auto path = cmd.get("path");

    dispatch_async(queue, ^{
      core.fsReadDir(seq, path, [&](auto seq, auto msg, auto post) {
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
    auto msg = self.core.getNetworkInterfaces();
    [self send: seq msg: msg post: Post{} ];
    return;
  }

  if (cmd.name == "readStop") {
    auto clientId = std::stoll(cmd.get("clientId"));

    dispatch_async(queue, ^{
      core.readStop(seq, clientId, [&](auto seq, auto msg, auto post) {
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
      core.shutdown(seq, clientId, [&](auto seq, auto msg, auto post) {
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
      core.sendBufferSize(seq, clientId, size, [&](auto seq, auto msg, auto post) {
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
      core.recvBufferSize(seq, clientId, size, [&](auto seq, auto msg, auto post) {
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
      core.close(seq, clientId, [&](auto seq, auto msg, auto post) {
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
      core.udpSend(seq, clientId, buf, offset, len, port, (const char*) ip.c_str(), [&](auto seq, auto msg, auto post) {
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
      core.tcpSend(clientId, buf, [&](auto seq, auto msg, auto post) {
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
      core.tcpConnect(seq, clientId, port, ip, [&](auto seq, auto msg, auto post) {
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
      core.tcpSetKeepAlive(seq, clientId, timeout, [&](auto seq, auto msg, auto post) {
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
      core.tcpSetTimeout(seq, clientId, timeout, [&](auto seq, auto msg, auto post) {
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
      core.tcpBind(seq, serverId, ip, port, [&](auto seq, auto msg, auto post) {
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
      core.dnsLookup(seq, hostname, [&](auto seq, auto msg, auto post) {
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

//    dispatch_async(dispatch_get_main_queue(), ^{
//      [self emit: name message: SSC::format(R"JSON({
//        "message": "$S"
//      })JSON", message)];
//    });
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
//  [self emit: "protocol" message: SSC::format(R"JSON({
//    "url": "$S",
//  })JSON", str)];

  return YES;
}

- (BOOL) application: (UIApplication*)application didFinishLaunchingWithOptions: (NSDictionary*)launchOptions {
  using namespace SSC;

  Core core;
  self.core = core;

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
