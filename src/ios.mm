#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Network/Network.h>
#import "common.hh"
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "lib/uv/include/uv.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>

#define DEFAULT_BACKLOG 128

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

static dispatch_queue_t queue = dispatch_queue_create("opkit.queue", DISPATCH_QUEUE_CONCURRENT);

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

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WKWebView*) webView decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, atomic) NSMutableDictionary* connections;
@property (strong, nonatomic) WKUserContentController* content;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
- (void) emit: (std::string)event message: (std::string)message;
- (void) resolve: (std::string)seq message: (std::string)message;
- (void) reject: (std::string)seq message: (std::string)message;
- (void) createServer: (std::string)seq address: (std::string)address port: (int)port;
- (void) connect: (std::string)seq port: (int)port address: (std::string)address;
- (void) send: (uint64_t)clientId message: (std::string)message;
@end

struct Context {
  uint64_t serverId;
  AppDelegate* delegate;
};

struct Client {
  uint64_t clientId;
  Context* ctx;
  uv_tcp_t *connection;
  std::string seq;
};

std::map<uint64_t, Client*> clients;

struct sockaddr_in addr;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

uv_loop_t *loop = uv_default_loop();
bool isRunning = false;

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

- (void) send: (uint64_t)clientId message: (std::string)message {
  Client* client = clients[clientId];

  if (client == nullptr) {
    return; // TODO tell the user
  }

  write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
  req->buf = uv_buf_init((char* const) message.c_str(), message.size());

  auto onWrite = [](uv_write_t *req, int status) {
    auto client = reinterpret_cast<Client*>(req->data);

    if (status) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [client->ctx->delegate emit: "data" message: Opkit::format(R"({
          "err": {
            "id": "$S",
            "message": "Write error $S"
          }
        })", uv_strerror(status))];
      });
      return;
    }

    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
  };

  uv_write((uv_write_t*) req, (uv_stream_t*) client->connection, &req->buf, 1, onWrite);
}

- (void) connect: (std::string)seq port: (int)port address: (std::string)address {
  dispatch_async(queue, ^{
    Context ctx;
    ctx.delegate = self;

    uv_tcp_t socket;
    socket.data = &ctx;

    uv_tcp_init(loop, &socket);
    uv_tcp_keepalive(&socket, 1, 60);

    auto clientId = rand64();
    Client* client = clients[clientId] = new Client();
    client->clientId = clientId;
    client->ctx = &ctx;
    client->seq = seq;
    client->connection = &socket;

    socket.data = &client;

    struct sockaddr_in dest;
    uv_ip4_addr(address.c_str(), port, &dest);

    auto onConnect = [](uv_connect_t* req, int status) {
      auto client = reinterpret_cast<Client*>(req->handle->data);

      if (status < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [client->ctx->delegate resolve: client->seq message: Opkit::format(R"({
            "err": {
              "clientId": "$S",
              "message": "$S"
            }
          })", std::to_string(client->clientId), std::string(uv_strerror(status)))];
        });
        return;
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        [client->ctx->delegate resolve: client->seq message: Opkit::format(R"({
          "data": {
            "clientId": "$S"
          }
        })", std::to_string(client->clientId))];
      });

      auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
        auto client = reinterpret_cast<Client*>(handle->data);

        NSString* str = [NSString stringWithUTF8String: buf->base];
        NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
        NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

        auto serverId = std::to_string(client->ctx->serverId);
        auto clientId = std::to_string(client->clientId);
        auto message = std::string([base64Encoded UTF8String]);

        dispatch_async(dispatch_get_main_queue(), ^{
          [client->ctx->delegate emit: "data" message: Opkit::format(R"({
            "data": {
              "serverId": "$S",
              "clientId": "$S",
              "message": "$S"
            }
          })", serverId, clientId, message)];
        });
      };

      auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = (char*) malloc(suggested_size);
        buf->len = suggested_size;
      };

      uv_read_start((uv_stream_t*) req->handle, allocate, onRead);
    };

    uv_connect_t connection;
    auto r = uv_tcp_connect(&connection, &socket, (const struct sockaddr*) &dest, onConnect);

    if (r) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: Opkit::format(R"({
          "err": {
            "clientId": "$S",
            "message": "$S"
          }
        })", std::to_string(clientId), std::string(uv_strerror(r)))];
      });

      return;
    }

    if (isRunning == false) {
      isRunning = true;
      NSLog(@"running new loop from request");
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

- (void) createServer: (std::string) seq address: (std::string)address port: (int)port {
  dispatch_async(queue, ^{
    Context ctx;
    ctx.delegate = self;
    ctx.serverId = rand64();

    uv_tcp_t server;
    server.data = &ctx;

    uv_tcp_init(loop, &server);
    uv_ip4_addr(address.c_str(), port, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*) &addr, 0);

    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, [](uv_stream_t *server, int status) {
      auto ctx = reinterpret_cast<Context*>(server->data);

      if (status < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [ctx->delegate emit: "data" message: Opkit::format(R"({
            "err": {
              "id": "$S",
              "message": "New connection error $S"
            }
          })", std::to_string(ctx->serverId), uv_strerror(status))];
        });
        return;
      }

      uv_tcp_t *connection = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));

      auto clientId = rand64();
      Client* client = clients[clientId] = new Client();
      client->clientId = clientId;
      client->ctx = ctx;
      client->connection = connection;

      connection->data = client;

      uv_tcp_init(loop, connection);

      if (uv_accept(server, (uv_stream_t*) connection) == 0) {
        auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf) {
          auto client = reinterpret_cast<Client*>(handle->data);

          if (nread > 0) {
            write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
            req->buf = uv_buf_init(buf->base, nread);

            NSString* str = [NSString stringWithUTF8String: req->buf.base];
            NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
            NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

            auto serverId = std::to_string(client->ctx->serverId);
            auto clientId = std::to_string(client->clientId);
            auto message = std::string([base64Encoded UTF8String]);

            dispatch_async(dispatch_get_main_queue(), ^{
              [client->ctx->delegate emit: "data" message: Opkit::format(R"({
                "data": {
                  "serverId": "$S",
                  "clientId": "$S",
                  "message": "$S"
                }
              })", serverId, clientId, message)];

              // echo it
              // [client->ctx->delegate send:client->clientId message:std::string(buf->base)];
            });
            return;
          }

          if (nread < 0) {
            if (nread != UV_EOF) {
              dispatch_async(dispatch_get_main_queue(), ^{
                [client->ctx->delegate emit: "data" message: Opkit::format(R"({
                  "err": {
                    "serverId": "$S",
                    "message": "$S"
                  }
                })", std::to_string(client->ctx->serverId), uv_err_name(nread))];
              });
            }

            uv_close((uv_handle_t*) client->connection, [](uv_handle_t* handle) {
              free(handle);
            });
          }

          free(buf->base);
        };

        auto allocateBuffer = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
          buf->base = (char*) malloc(suggested_size);
          buf->len = suggested_size;
        };

        uv_read_start((uv_stream_t*) connection, allocateBuffer, onRead);
      } else {
        uv_close((uv_handle_t*) connection, [](uv_handle_t* handle) {
          free(handle);
        });
      }
    });

    if (r) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: Opkit::format(R"({
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        })", std::to_string(ctx.serverId), std::string(uv_strerror(r)))];
      });

      return;
    }

    if (isRunning == false) {
      isRunning = true;
      NSLog(@"running new loop from server");
      uv_run(loop, UV_RUN_DEFAULT);
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve: seq message: Opkit::format(R"({
        "data": {
          "serverId": "$S",
          "port": "$S",
          "address": "$S"
        }
      })", std::to_string(ctx.serverId), port, address)];
    });
  });
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

      NSLog(@"COMMAND %s", msg.c_str());

      if (cmd.name == "openExternal") {
        // NSString *url = [NSString stringWithUTF8String:cmd.value.c_str()];
        // [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
        return;
      }

      if (cmd.name == "getAddress") {
        auto hostname = std::string([[self getIPAddress] UTF8String]);

        [self resolve: cmd.get("seq") message: Opkit::format(R"({
          "data": {
            "address": "$S"
          }
        })", hostname)];

        return;
      }

      if (cmd.name == "send") {
        [self send: std::stoll(cmd.get("clientId")) message: cmd.get("message")];
        return;
      }

      if (cmd.name == "connect") {
        auto address = cmd.get("address");
        auto port = cmd.get("port");
        auto seq = cmd.get("seq");

        [self connect: seq port:std::stoi(port) address:address];
        return;
      }

      if (cmd.name == "createServer") {
        auto address = cmd.get("address");
        auto port = cmd.get("port");
        auto seq = cmd.get("seq");

        if (address.size() == 0) {
          address = std::string([[self getIPAddress] UTF8String]);
        }

        if (address == "error") {
          [self resolve: cmd.get("seq") message: Opkit::format(R"({
            "err": { "message": "offline" }
          })")];
          return;
        }

        if (port.size() == 0) {
          [self reject: cmd.get("seq") message: Opkit::format(R"({
            "err": { "message": "port required" }
          })")];
          return;
        }

        [self createServer: seq address: address port: std::stoi(port)];

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

