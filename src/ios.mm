#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import "common.hh"
#include <ifaddrs.h>
#include <arpa/inet.h>

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

char *g_psk = nullptr;      // TLS PSK
char *g_local_port = nullptr;  // Local port flag
char *g_local_addr = nullptr;  // Source Address
bool g_use_bonjour = false;  // Use Bonjour rather than hostnames
bool g_detached = false;  // Ignore stdin
bool g_listener = false;  // Create a listener
bool g_use_tls = false;    // Use TLS or DTLS
bool g_use_udp = false;    // Use UDP instead of TCP
int g_family = AF_UNSPEC;   // Required address family
std::vector<nw_connection_t> connections;

// nw_connection_t g_inbound_connection = nullptr;

#define NWCAT_BONJOUR_SERVICE_TCP_TYPE "_opkit._tcp"
#define NWCAT_BONJOUR_SERVICE_UDP_TYPE "_opkit._udp"
#define NWCAT_BONJOUR_SERVICE_DOMAIN "local"

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

class NetworkConnect;

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
// @property (strong, nonatomic) nw_listener_t listener;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@property NetworkConnect* connector;

// @property (nonatomic) bool connected;
@property (nonatomic) nw_connection_t listener;
// @property (nonatomic) const char* dns;
// @property (nonatomic) uint16_t tls_encryption;
// @property (nonatomic) const char* ip;
// @property (nonatomic) const char* port;
@end

class NetworkConnect {
  public:
    AppDelegate* parent;
    void receiveData(nw_connection_t connection, std::string streamId);
    void sendData(nw_connection_t connection, std::string seq, std::string buffer);
    void netBind(std::string seq, char* name, char* port);
    std::string getLocalIP ();
    NetworkConnect (AppDelegate* ad) : parent(ad) {}
};

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate

@synthesize listener;

- (void) eval: (std::string)message {
  auto script = [NSString stringWithUTF8String:message.c_str()];
  [self.webview evaluateJavaScript: script completionHandler:nil];
}

- (NSString*) getIPAddress {
  NSString *address = @"error";
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  int success = 0;
  // retrieve the current interfaces - returns 0 on success
  success = getifaddrs(&interfaces);

  if (success == 0) {
      // Loop through linked list of interfaces
      temp_addr = interfaces;
      while(temp_addr != NULL) {
          if(temp_addr->ifa_addr->sa_family == AF_INET) {
              // Check if interface is en0 which is the wifi connection on the iPhone
              if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"]) {
                  // Get NSString from C String
                  address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];

              }

          }

          temp_addr = temp_addr->ifa_next;
      }
  }
  // Free memory
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

      if (cmd.name == "netBind") {
        if (self.connector) {
          [self eval: resolveToRenderProcess(cmd.get("seq"), "0", "EXISTS")];
          return;
        }

        g_family = cmd.get("family").find("ipv6") == 0 ? AF_INET6 : AF_INET;
        g_use_bonjour = cmd.get("bonjour").find("true") == 0 ? true : false;
        g_use_tls = cmd.get("tls").find("true") == 0 ? true : false;
        g_use_udp = cmd.get("udp").find("true") == 0 ? true : false;

        self.connector = new NetworkConnect(self);
        auto hostname = cmd.get("hostname");

        if (hostname.size() == 0) {
          hostname = std::string([[self getIPAddress] UTF8String]);
        }

        NSLog(@"ip %s", hostname.c_str());

        self.connector->netBind(
          cmd.get("seq"),
          (char*) hostname.c_str(),
          (char*) cmd.get("port").c_str()
        );

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

//
// receive_loop()
// Perform a single read on the supplied connection, and write data to
// stdout as it is received.
// If no error is encountered, schedule another read on the same connection.
//
void NetworkConnect::receiveData (nw_connection_t connection, std::string streamId) {
  nw_connection_receive(connection, 1, UINT32_MAX, ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {

    // nw_retain(context);
    dispatch_block_t schedule_next_receive = ^{
      // If the context is marked as complete, and is the final context,
      // we're read-closed.
      if (is_complete &&
        (context == nullptr || nw_content_context_get_is_final(context))) {
        [this->parent eval: Opkit::streamToRenderProcess(streamId, "\0")];
      }

      // If there was no error in receiving, request more data... possibly limit the number of times this can happen before failing.
      if (receive_error == nullptr) {
        this->receiveData(connection, streamId);
      }
      // nw_release(context);
    };

    if (content != nullptr) {
      NSMutableString* str = [NSMutableString string];

      dispatch_data_apply(content, ^bool(dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
        [str appendString: [[NSString alloc] initWithBytes:buffer length:size encoding: NSUTF8StringEncoding]];
        return true;
      });

      [this->parent eval: Opkit::streamToRenderProcess(streamId, std::string([str UTF8String]))];
    }

    schedule_next_receive();
  });
}

//
// send_loop()
// Start reading from stdin on a dispatch source, and send any bytes on the given connection.
//
void NetworkConnect::sendData (nw_connection_t connection, std::string seq, std::string buffer) {
  if (!g_detached) {
    dispatch_data_t read_data = dispatch_data_create(buffer.c_str(), (size_t) buffer.size(), dispatch_get_main_queue(), ^{});

    if (read_data == nullptr) {
      // NULL data represents EOF
      // Send a "write close" on the connection, by sending NULL data with the final message context marked as complete.
      // Note that it is valid to send with NULL data but a non-NULL context.
      nw_connection_send(connection, NULL, NW_CONNECTION_FINAL_MESSAGE_CONTEXT, true, ^(nw_error_t  _Nullable error) {
        if (error != nullptr) {
          errno = nw_error_get_error_code(error);
          [this->parent eval: Opkit::resolveToRenderProcess(seq, "1", "NOT OK")];
        }
      });
    } else {
      // Every send is marked as complete. This has no effect with the default message context for TCP,
      // but is required for UDP to indicate the end of a packet.
      nw_connection_send(connection, read_data, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t  _Nullable error) {
        if (error != nullptr) {
          errno = nw_error_get_error_code(error);
          [this->parent eval: Opkit::resolveToRenderProcess(seq, "1", "NOT OK")];
        } else {
          [this->parent eval: Opkit::resolveToRenderProcess(seq, "0", "OK")];
        }
      });
    }
  }
}

void NetworkConnect::netBind (std::string seq, char* name, char* port) {
  nw_parameters_t parameters = nullptr;

  // nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DEFAULT_CONFIGURATION;
  nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DISABLE_PROTOCOL;

  if (g_use_udp) {
    parameters = nw_parameters_create_secure_udp(configure_tls, NW_PARAMETERS_DEFAULT_CONFIGURATION);
  } else {
    parameters = nw_parameters_create_secure_tcp(configure_tls, NW_PARAMETERS_DEFAULT_CONFIGURATION);
  }

  nw_protocol_stack_t protocol_stack = nw_parameters_copy_default_protocol_stack(parameters);

  if (g_family == AF_INET || g_family == AF_INET6) {
    nw_protocol_options_t ip_options = nw_protocol_stack_copy_internet_protocol(protocol_stack);

    if (g_family == AF_INET) {
      nw_ip_options_set_version(ip_options, nw_ip_version_4);
    } else if (g_family == AF_INET6) {
      nw_ip_options_set_version(ip_options, nw_ip_version_6);
    }
  }

  const char *address = g_use_bonjour ? nullptr : name;

  if (address || port) {
    nw_endpoint_t local_endpoint = nw_endpoint_create_host(address ? address : "::", port ? port : "0");
    nw_parameters_set_fast_open_enabled(parameters, true);
    nw_parameters_set_include_peer_to_peer(parameters, true);
    nw_parameters_set_local_endpoint(parameters, local_endpoint);
    NSLog(@"host and port - %@", local_endpoint);
  }

  nw_listener_t listener = nw_listener_create(parameters);

  if (g_use_bonjour && name != nullptr) {
    auto serviceType = (g_use_udp ? NWCAT_BONJOUR_SERVICE_UDP_TYPE : NWCAT_BONJOUR_SERVICE_TCP_TYPE);
    nw_advertise_descriptor_t advertise = nw_advertise_descriptor_create_bonjour_service(
      name,
      serviceType,
      NWCAT_BONJOUR_SERVICE_DOMAIN
    );

    nw_listener_set_advertise_descriptor(listener, advertise);

    nw_listener_set_advertised_endpoint_changed_handler(listener, ^(nw_endpoint_t _Nonnull advertised_endpoint, bool added) {
      NSLog(@"Listener %s on %s (%s.%s.%s)\n",
        added ? "added" : "removed",
        nw_endpoint_get_bonjour_service_name(advertised_endpoint),
        nw_endpoint_get_bonjour_service_name(advertised_endpoint),
        serviceType,
        NWCAT_BONJOUR_SERVICE_DOMAIN);
    });
  }

  nw_listener_set_queue(listener, dispatch_get_main_queue());

  nw_listener_set_state_changed_handler(listener, ^(nw_listener_state_t state, nw_error_t error) {
    errno = error ? nw_error_get_error_code(error) : 0;
    if (state == nw_listener_state_waiting) {
      NSLog(@"Listener on port %u (%s) waiting\n", nw_listener_get_port(listener), g_use_udp ? "udp" : "tcp");
      [this->parent eval: Opkit::resolveToRenderProcess(seq, "0", Opkit::encodeURIComponent("{\"data\":\"WAITING\"}"))];
    } else if (state == nw_listener_state_failed) {
      NSLog(@"listener (%s) failed", g_use_udp ? "udp" : "tcp");
      [this->parent eval: Opkit::resolveToRenderProcess(seq, "0", Opkit::encodeURIComponent("{\"err\":\"FAILED\"}"))];
    } else if (state == nw_listener_state_ready) {
      NSLog(@"Listener on port %u (%s) ready!\n", nw_listener_get_port(listener), g_use_udp ? "udp" : "tcp");
      [this->parent eval: Opkit::resolveToRenderProcess(seq, "0", Opkit::encodeURIComponent("{\"data\":\"READY\"}"))];
    } else if (state == nw_listener_state_cancelled) {
      // Release the primary reference on the listener
      // that was taken at creation time
      // nw_release(listener);
      [this->parent eval: Opkit::resolveToRenderProcess(seq, "0", Opkit::encodeURIComponent("{\"err\":\"CANCELLED\"}"))];
    }
  });

  if (g_family == AF_INET) {
    NSLog(@"xxx - ipv4");
  }

  NSLog(@"%@", listener);

  nw_listener_set_new_connection_handler(listener, ^(nw_connection_t connection) {
    NSLog(@"xxx - 2a");
    nw_connection_set_queue(connection, dispatch_get_main_queue());

    // rw_retain(connection); // Hold a reference until cancelled
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
      nw_endpoint_t remote = nw_connection_copy_endpoint(connection);
      errno = error ? nw_error_get_error_code(error) : 0;

      if (state == nw_connection_state_waiting) {
        NSLog(@"connect to %s port %u (%s) failed, is waiting",
         nw_endpoint_get_hostname(remote),
         nw_endpoint_get_port(remote),
         g_use_udp ? "udp" : "tcp");
        [this->parent eval: Opkit::emitToRenderProcess("connection", Opkit::encodeURIComponent("{\"err\":\"WAITING\"}"))];
      } else if (state == nw_connection_state_failed) {
        NSLog(@"connect to %s port %u (%s) failed",
         nw_endpoint_get_hostname(remote),
         nw_endpoint_get_port(remote),
         g_use_udp ? "udp" : "tcp");
        [this->parent eval: Opkit::emitToRenderProcess("connection", Opkit::encodeURIComponent("{\"err\":\"FAILED\"}"))];
      } else if (state == nw_connection_state_ready) {
        NSLog(@"Connection to %s port %u (%s) succeeded!\n",
          nw_endpoint_get_hostname(remote),
          nw_endpoint_get_port(remote),
          g_use_udp ? "udp" : "tcp");
        [this->parent eval: Opkit::emitToRenderProcess("connection", Opkit::encodeURIComponent("{\"data\":\"OK\"}"))];
      } else if (state == nw_connection_state_cancelled) {
        [this->parent eval: Opkit::emitToRenderProcess("connection", Opkit::encodeURIComponent("{\"err\":\"CANCELLED\"}"))];
        // nw_release(connection);
      }
      // nw_release(remote);
    });

    NSLog(@"xxx - 3");

    nw_connection_start(connection);

    this->sendData(connection, seq, "");
    this->receiveData(connection, "");
    NSLog(@"xxx - 4");
  });

  nw_listener_start(listener);
}

