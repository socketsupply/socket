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

dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("opkit.queue", qos);

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webView: (WKWebView*)webView
    decidePolicyForNavigationAction: (WKNavigationAction*)navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@property nw_path_monitor_t monitor;
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
- (void) emit: (std::string)event message: (std::string)message;
- (void) resolve: (std::string)seq message: (std::string)message;
- (void) reject: (std::string)seq message: (std::string)message;
- (void) tcpCreateServer: (std::string)seq address: (std::string)address port: (int)port;
- (void) tcpConnect: (std::string)seq port: (int)port address: (std::string)address;
- (void) tcpSend: (uint64_t)clientId message: (std::string)message;
- (void) udpBind: (std::string)seq address: (std::string)address port: (int)port;
- (void) udpConnect: (std::string)seq port: (int)port address: (std::string)address;
- (void) udpSend: (uint64_t)clientId message: (std::string)message;
- (void) closeConnection: (std::string)seq connectionId: (uint64_t)connectionId;
@end

struct Context {
  uint64_t serverId;
  AppDelegate* delegate;
};

struct Client {
  uint64_t clientId;
  AppDelegate* delegate;
  Context* ctx;
  uv_tcp_t *connection; // tcp is a connection
  uv_udp_t *socket; // udp is connectionless
  std::string seq;
};

std::string addrToIPv4 (struct sockaddr_in* addr) {
  char buf[INET_ADDRSTRLEN];
  struct sockaddr_in *sin = (struct sockaddr_in*) &addr;
  inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
  return std::string(buf);
}

std::string addrToIPv6 (struct sockaddr_in6* addr) {
  char buf[INET6_ADDRSTRLEN];
  struct sockaddr_in6 *sin = (struct sockaddr_in6*) &addr;
  inet_ntop(AF_INET6, &sin->sin6_addr, buf, sizeof(buf));
  return std::string(buf);
}

struct PeerInfo {
  std::string address = "";
  std::string family = "";
  int port = 0;
  int error = 0;
  void init(uv_tcp_t* connection);
  void init(uv_udp_t* socket);
};

void PeerInfo::init (uv_tcp_t* connection) {
  int namelen;
  struct sockaddr_storage addr;
  namelen = sizeof(addr);

  error = uv_tcp_getpeername(connection, (struct sockaddr*) &addr, &namelen);

  if (error) {
    return;
  }

  if (addr.ss_family == AF_INET) {
    family = "ipv4";
    address = addrToIPv4((struct sockaddr_in*) &addr);
    port = (int) htons(((struct sockaddr_in*) &addr)->sin_port);
  } else {
    family = "ipv6";
    address = addrToIPv6((struct sockaddr_in6*) &addr);
    port = (int) htons(((struct sockaddr_in6*) &addr)->sin6_port);
  }
}

void PeerInfo::init (uv_udp_t* socket) {
  int namelen;
  struct sockaddr_storage addr;
  namelen = sizeof(addr);

  error = uv_udp_getpeername(socket, (struct sockaddr*) &addr, &namelen);

  if (error) {
    return;
  }

  if (addr.ss_family == AF_INET) {
    family = "ipv4";
    address = addrToIPv4((struct sockaddr_in*) &addr);
    port = (int) htons(((struct sockaddr_in*) &addr)->sin_port);
  } else {
    family = "ipv6";
    address = addrToIPv6((struct sockaddr_in6*) &addr);
    port = (int) htons(((struct sockaddr_in6*) &addr)->sin6_port);
  }
}

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
- (std::string) getNetworkInterfaces {
  struct ifaddrs *interfaces = nullptr;
  struct ifaddrs *addr = nullptr;
  int success = getifaddrs(&interfaces);
  std::stringstream data;
  std::stringstream v4;
  std::stringstream v6;

  if (success != 0) return "";

  addr = interfaces;
  v4 << "\"ipv4\":{";
  v6 << "\"ipv6\":{";

  while (addr != nullptr) {
    std::string ip = "";

    if (addr->ifa_addr->sa_family == AF_INET) {
      ip = addrToIPv4((struct sockaddr_in*) &addr->ifa_addr);

      if (ip.size() > 0) {
        v4 << "\"" << addr->ifa_name << "\": \"" << ip << "\",";
      }
    }

    if (addr->ifa_addr->sa_family == AF_INET6) {
      ip = addrToIPv6((struct sockaddr_in6*) &addr->ifa_addr);

      if (ip.size() > 0) {
        v6 << "\"" << addr->ifa_name << "\": \"" << ip << "\",";
      }
    }

    addr = addr->ifa_next;
  }

  v4 << "\"local\":\"0.0.0.0\"}";
  v6 << "\"local\":\"::1\"}";

  getifaddrs(&interfaces);
  freeifaddrs(interfaces);

  data << "{" << v4.str() << "," << v6.str() << "}";
  return data.str();
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

- (void) tcpSend: (uint64_t)clientId message: (std::string)message {
  Client* client = clients[clientId];

  if (client == nullptr) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self emit: "error" message: Opkit::format(R"JSON({
        "clientId": "$S",
        "data": {
          "message": "Not connected"
        }
      })JSON", std::to_string(clientId))];
    });
    return;
  }

  write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
  req->buf = uv_buf_init((char* const) message.c_str(), message.size());

  auto onWrite = [](uv_write_t *req, int status) {
    auto client = reinterpret_cast<Client*>(req->data);

    if (status) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [client->ctx->delegate emit: "error" message: Opkit::format(R"({
          "clientId": "$S",
          "data": {
            "message": "Write error $S"
          }
        })", std::to_string(client->clientId), uv_strerror(status))];
      });
      return;
    }

    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
  };

  uv_write((uv_write_t*) req, (uv_stream_t*) client->connection, &req->buf, 1, onWrite);
}

- (void) tcpConnect: (std::string)seq port: (int)port address: (std::string)address {
  dispatch_async(queue, ^{
    uv_tcp_t connection;
    uv_connect_t connect;

    auto clientId = Opkit::rand64();
    Client* client = clients[clientId] = new Client();

    client->delegate = self;
    client->clientId = clientId;

    uv_tcp_init(loop, &connection);

    connection.data = client;

    uv_tcp_init(loop, &connection);
    uv_tcp_nodelay(&connection, 0);
    uv_tcp_keepalive(&connection, 1, 60);

    struct sockaddr_in dest4;
    struct sockaddr_in6 dest6;

    if (address.find(":") != std::string::npos) {
      uv_ip6_addr(address.c_str(), port, &dest6);
    } else {
      uv_ip4_addr(address.c_str(), port, &dest4);
    }

    // uv_ip4_addr("172.217.16.46", 80, &dest);

    NSLog(@"connect? %s:%i", address.c_str(), port);

    auto onConnect = [](uv_connect_t* connect, int status) {
      auto* client = reinterpret_cast<Client*>(connect->handle->data);

      NSLog(@"client connection?");

      if (status < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          auto clientId = std::to_string(client->clientId);
          auto error = std::string(uv_strerror(status));

          auto message = Opkit::format(R"({
            "err": {
              "clientId": "$S",
              "message": "$S"
            }
          })", clientId, error);

          [client->delegate resolve: client->seq message: message];
        });
        return;
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        [client->delegate resolve: client->seq message: Opkit::format(R"({
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

        auto clientId = std::to_string(client->clientId);
        auto message = std::string([base64Encoded UTF8String]);

        dispatch_async(dispatch_get_main_queue(), ^{
          [client->delegate emit: "data" message: Opkit::format(R"({
            "clientId": "$S",
            "data": "$S"
          })", clientId, message)];
        });
      };

      auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = (char*) malloc(suggested_size);
        buf->len = suggested_size;
      };

      uv_read_start((uv_stream_t*) connect->handle, allocate, onRead);
    };

    int r = 0;

    if (address.find(":") != std::string::npos) {
      r = uv_tcp_connect(&connect, &connection, (const struct sockaddr*) &dest6, onConnect);
    } else {
      r = uv_tcp_connect(&connect, &connection, (const struct sockaddr*) &dest4, onConnect);
    }

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
      NSLog(@"Starting loop from request");
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

- (void) tcpCreateServer: (std::string) seq address: (std::string)address port: (int)port {
  dispatch_async(queue, ^{
    loop = uv_default_loop();
    Context ctx;
    ctx.delegate = self;
    ctx.serverId = Opkit::rand64();

    uv_tcp_t server;
    server.data = &ctx;

    uv_tcp_init(loop, &server);
    struct sockaddr_in addr;
    // addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // NSLog(@"LISTENING %i", addr.sin_addr.s_addr);
    // NSLog(@"LISTENING %s:%i", address.c_str(), port);
    uv_ip4_addr(address.c_str(), port, &addr);
    uv_tcp_simultaneous_accepts(&server, 0);
    uv_tcp_bind(&server, (const struct sockaddr*) &addr, 0);

    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, [](uv_stream_t *server, int status) {
      auto ctx = reinterpret_cast<Context*>(server->data);

      NSLog(@"connection?");

      if (status < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [ctx->delegate emit: "connection" message: Opkit::format(R"JSON({
            "serverId": "$S",
            "data": "New connection error $S"
          })JSON", std::to_string(ctx->serverId), uv_strerror(status))];
        });
        return;
      }

      uv_tcp_t* connection = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));

      auto clientId = Opkit::rand64();
      Client* client = clients[clientId] = new Client();
      client->clientId = clientId;
      client->ctx = ctx;
      client->connection = connection;

      connection->data = client;

      uv_tcp_init(loop, connection);

      if (uv_accept(server, (uv_stream_t*) connection) == 0) {
        PeerInfo info;
        info.init(connection);

        dispatch_async(dispatch_get_main_queue(), ^{
          [ctx->delegate emit: "connection" message: Opkit::format(R"JSON({
            "serverId": "$S",
            "data": {
              "address": "$S",
              "family": "$S",
              "port": "$i"
            }
          })JSON", std::to_string(ctx->serverId), info.address, info.family, info.port)];
        });

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
              [client->ctx->delegate emit: "data" message: Opkit::format(R"JSON({
                "serverId": "$S",
                "clientId": "$S",
                "data": "$S"
              })JSON", serverId, clientId, message)];

              // echo it
              // [client->ctx->delegate send:client->clientId message:std::string(buf->base)];
            });
            return;
          }

          if (nread < 0) {
            if (nread != UV_EOF) {
              dispatch_async(dispatch_get_main_queue(), ^{
                [client->ctx->delegate emit: "error" message: Opkit::format(R"JSON({
                  "serverId": "$S",
                  "data": "$S"
                })JSON", std::to_string(client->ctx->serverId), uv_err_name(nread))];
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
        [self resolve: seq message: Opkit::format(R"JSON({
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        })JSON", std::to_string(ctx.serverId), std::string(uv_strerror(r)))];
      });

      NSLog(@"Listener failed: %s", uv_strerror(r));
      return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve: seq message: Opkit::format(R"JSON({
        "data": {
          "serverId": "$S",
          "port": "$i",
          "address": "$S"
        }
      })JSON", std::to_string(ctx.serverId), port, address)];
      NSLog(@"Listener started");
    });

    if (isRunning == false) {
      isRunning = true;
      NSLog(@"Starting loop from server");
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

- (void) closeConnection: (std::string)seq connectionId: (uint64_t)connectionId {
  Client* client = clients[connectionId];
  client->seq = seq;
  client->delegate = self;
  client->clientId = connectionId;

  if (client == nullptr) {
    [self resolve:seq message: Opkit::format(R"JSON({
      "err": {
        "message": "No connection with specified id ($S)"
      }
    })JSON", std::to_string(connectionId))];
    return;
  }

  dispatch_async(queue, ^{
    uv_close((uv_handle_t*) client->connection, [](uv_handle_t* handle) {
      auto client = reinterpret_cast<Client*>(handle->data);
      free(handle);

      [client->delegate resolve:client->seq message: Opkit::format(R"JSON({
        "data": {}
      })JSON")];
    });
  });
}

- (void) udpBind: (std::string)seq address: (std::string)address port: (int)port {

}
- (void) udpConnect: (std::string)seq port: (int)port address: (std::string)address {

}
- (void) udpClose: (std::string)seq connectionId: (uint64_t)connectionId {

}
- (void) udpSend: (uint64_t)clientId message: (std::string)message {

}

- (void) applicationDidEnterBackground {
  [self emit: "application" message: Opkit::format(R"JSON({
    "status": "background"
  })JSON")];
}

- (void) applicationWillEnterForeground {
  [self emit: "application" message: Opkit::format(R"JSON({
    "status": "foreground"
  })JSON")];
}

- (void) initNetworkStatusObserver {
dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_UTILITY, DISPATCH_QUEUE_PRIORITY_DEFAULT);
  self.monitorQueue = dispatch_queue_create("com.example.network-monitor", attrs);

  // self.monitor = nw_path_monitor_create_with_type(nw_interface_type_wifi);
  self.monitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(self.monitor, self.monitorQueue);
  nw_path_monitor_set_update_handler(self.monitor, ^(nw_path_t _Nonnull path) {
    nw_path_status_t status = nw_path_get_status(path);
    // Determine the active interface, but how?
    switch (status) {
      case nw_path_status_invalid: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "network" message: Opkit::format(R"JSON({
            "status": "offline",
            "message": "Network path is invalid"
          })JSON")];
        });
        break;
      }
      case nw_path_status_satisfied: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "network" message: Opkit::format(R"JSON({
            "status": "online",
            "message": "Network is usable"
          })JSON")];
        });
        break;
      }
      case nw_path_status_satisfiable: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "network" message: Opkit::format(R"JSON({
            "status": "online",
            "message": "Network may be usable"
          })JSON")];
        });
        break;
      }
      case nw_path_status_unsatisfied: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "network" message: Opkit::format(R"JSON({
            "status": "offline",
            "message": "Network is not usable"
          })JSON")];
        });
        break;
      }
    }
  });

  nw_path_monitor_start(self.monitor);
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

      if (cmd.name == "address") {
        auto clientId = cmd.get("clientId");
        auto seq = cmd.get("seq");

        if (clientId.size() == 0) {
          [self reject: seq message: Opkit::format(R"JSON({
            "err": {
              "message": "no clientid"
            }
          })JSON")];
        }

        Client* client = clients[std::stoll(clientId)];

        if (client == nullptr) {
          [self resolve: seq message: Opkit::format(R"JSON({
            "err": {
              "message": "not connected"
            }
          })JSON")];
        }

        PeerInfo info;
        info.init(client->connection);

        [self resolve: seq message: Opkit::format(R"JSON({
          "data": {
            "address": "$S",
            "family": "$S",
            "port": "$i"
          }
        })JSON", std::to_string(client->clientId), info.address, info.family, info.port)];

        return;
      }

      if (cmd.name == "getNetworkInterfaces") {
        auto seq = cmd.get("seq");
        auto message = Opkit::format(R"({
          "data": $S
        })", [self getNetworkInterfaces]);

        [self resolve: seq message: message];

        return;
      }

      if (cmd.name == "tpcSend") {
        [self tcpSend: std::stoll(cmd.get("clientId")) message: cmd.get("message")];
        return;
      }

      if (cmd.name == "tpcConnect") {
        auto address = cmd.get("address");
        auto port = cmd.get("port");
        auto seq = cmd.get("seq");

        [self tcpConnect: seq port: std::stoi(port) address: address];
        return;
      }

      if (cmd.name == "tpcCreateServer") {
        auto address = cmd.get("address");
        auto port = cmd.get("port");
        auto seq = cmd.get("seq");

        if (address.size() == 0) {
          address = "0.0.0.0";
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

        [self tcpCreateServer: seq address: address port: std::stoi(port)];

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

      "" + std::string(gEventEmitter) + "\n"
      "" + std::string(gStreams) + "\n"
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
    [self initNetworkStatusObserver];

    return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}

