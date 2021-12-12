#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "ios.hh"
#import "common.hh"

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

char *g_local_port = nullptr;  // Local port flag
char *g_local_addr = nullptr;  // Source Address
bool g_use_bonjour = false;  // Use Bonjour rather than hostnames
bool g_detached = false;  // Ignore stdin
bool g_listener = false;  // Create a listener
bool g_use_tls = false;    // Use TLS or DTLS
bool g_use_udp = false;    // Use UDP instead of TCP
bool g_verbose = false;    // Verbose
int g_family = AF_UNSPEC;   // Required address family

nw_connection_t g_inbound_connection = nullptr;

#define NWCAT_BONJOUR_SERVICE_TCP_TYPE "_nwcat._tcp"
#define NWCAT_BONJOUR_SERVICE_UDP_TYPE "_nwcat._udp"
#define NWCAT_BONJOUR_SERVICE_DOMAIN "local"

class PeerConnect {
  public:
    WKWebView* webview;
    void resolveToRenderProcess(std::string seq, std::string state, std::string value);
    void emitToRenderProcess(std::string event, std::string value);
    void receive(nw_connection_t connection);
    void send(nw_connection_t connection);
    nw_listener_t listen(char* name, char* port);
    PeerConnect (WKWebView* webview) : webview(webview) {}
};

void PeerConnect::resolveToRenderProcess (std::string seq, std::string state, std::string value) {
  this->webview.evaluateJavaScript(std::string(
    "(() => {"
    "  const seq = Number('" + seq + "');"
    "  const state = Number('" + state + "');"
    "  const value = '" + value + "';"
    "  window._ipc.resolve(seq, state, value);"
    "})()"
  ), nullptr);
}

void PeerConnect::emitToRenderProcess (std::string event, std::string value) {
  this->webview.evaluateJavaScript(std::string(
    "(() => {"
    "  const name = '" + event + "';"
    "  const value = '" + value + "';"
    "  window._ipc.emit(name, value);"
    "})()"
  ));
}

//
// receive_loop()
// Perform a single read on the supplied connection, and write data to
// stdout as it is received.
// If no error is encountered, schedule another read on the same connection.
//
void PeerConnect::receive (nw_connection_t connection) {
  nw_connection_receive(connection, 1, UINT32_MAX, ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t receive_error) {

    // nw_retain(context);
    dispatch_block_t schedule_next_receive = ^{
      // If the context is marked as complete, and is the final context,
      // we're read-closed.
      if (is_complete &&
        (context == NULL || nw_content_context_get_is_final(context))) {
        exit(0);
      }

      // If there was no error in receiving, request more data
      if (receive_error == NULL) {
        this->receive(connection);
      }
      // nw_release(context);
    };

    if (content != NULL) {
      NSMutableString* str = [NSMutableString string];

      dispatch_data_apply(content, ^bool(dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
        [str appendString: [[NSString alloc] initWithBytes:buffer length:size encoding: NSUTF8StringEncoding]];
        return true;
      });

      this->emitToRenderProcess("data", std::string((char*) str));

      dispatch_write(STDOUT_FILENO, content, dispatch_get_main_queue(), ^(__unused dispatch_data_t _Nullable data, int stdout_error) {
        if (stdout_error != 0) {
          errno = stdout_error;
          NSLog(@"stdout write error");
        } else {
          schedule_next_receive();
        }
        Block_release(schedule_next_receive);
      });
    }

    schedule_next_receive();
  });
}

//
// send_loop()
// Start reading from stdin on a dispatch source, and send any bytes on the given connection.
//
void PeerConnect::send (nw_connection_t connection) {
  if (!g_detached) {
    dispatch_read(STDIN_FILENO, 8192, dispatch_get_main_queue(), ^(dispatch_data_t _Nonnull read_data, int stdin_error) {
      if (stdin_error != 0) {
        errno = stdin_error;
        NSLog(@"stdin read error");
      } else if (read_data == NULL) {
        // NULL data represents EOF
        // Send a "write close" on the connection, by sending NULL data with the final message context marked as complete.
        // Note that it is valid to send with NULL data but a non-NULL context.
        nw_connection_send(connection, NULL, NW_CONNECTION_FINAL_MESSAGE_CONTEXT, true, ^(nw_error_t  _Nullable error) {
          if (error != NULL) {
            errno = nw_error_get_error_code(error);
            NSLog(@"write close error");
          }
          // Stop reading from stdin, so don't schedule another send_loop
        });
      } else {
        // Every send is marked as complete. This has no effect with the default message context for TCP,
        // but is required for UDP to indicate the end of a packet.
        nw_connection_send(connection, read_data, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t  _Nullable error) {
          if (error != NULL) {
            errno = nw_error_get_error_code(error);
            NSLog(@"send error");
          } else {
            // Continue reading from stdin
            this->send(connection);
          }
        });
      }
    });
  }
}

nw_listener_t PeerConnect::listen (char* name, char* port) {
  nw_parameters_t parameters = nullptr;

  nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DISABLE_PROTOCOL;
  if (g_use_tls) {
    configure_tls = NW_PARAMETERS_DEFAULT_CONFIGURATION;
  }

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
    nw_parameters_set_local_endpoint(parameters, local_endpoint);
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
      if (g_verbose) {
        NSLog(@"Listener %s on %s (%s.%s.%s)\n",
          added ? "added" : "removed",
          nw_endpoint_get_bonjour_service_name(advertised_endpoint),
          nw_endpoint_get_bonjour_service_name(advertised_endpoint),
          serviceType,
          NWCAT_BONJOUR_SERVICE_DOMAIN);
      }
    });
  }

  nw_listener_set_queue(listener, dispatch_get_main_queue());

  nw_listener_set_state_changed_handler(listener, ^(nw_listener_state_t state, nw_error_t error) {
    errno = error ? nw_error_get_error_code(error) : 0;
    if (state == nw_listener_state_waiting) {
      if (g_verbose) {
        NSLog(@"Listener on port %u (%s) waiting\n", nw_listener_get_port(listener), g_use_udp ? "udp" : "tcp");
      }
    } else if (state == nw_listener_state_failed) {
      NSLog(@"listener (%s) failed", g_use_udp ? "udp" : "tcp");
    } else if (state == nw_listener_state_ready) {
      if (g_verbose) {
        NSLog(@"Listener on port %u (%s) ready!\n", nw_listener_get_port(listener), g_use_udp ? "udp" : "tcp");
      }
    } else if (state == nw_listener_state_cancelled) {
      // Release the primary reference on the listener
      // that was taken at creation time
      // nw_release(listener);
      NSLog(@"Listener cancelled");
    }
  });

  nw_listener_set_new_connection_handler(listener, ^(nw_connection_t connection) {
    if (g_inbound_connection != nullptr) {
      // We only support one connection at a time, so if we already
      // have one, reject the incoming connection.
      nw_connection_cancel(connection);
      return;
    }

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
      } else if (state == nw_connection_state_failed) {
        NSLog(@"connect to %s port %u (%s) failed",
         nw_endpoint_get_hostname(remote),
         nw_endpoint_get_port(remote),
         g_use_udp ? "udp" : "tcp");
      } else if (state == nw_connection_state_ready) {
        if (g_verbose) {
          NSLog(@"Connection to %s port %u (%s) succeeded!\n",
            nw_endpoint_get_hostname(remote),
            nw_endpoint_get_port(remote),
            g_use_udp ? "udp" : "tcp");
        }
      } else if (state == nw_connection_state_cancelled) {
        // Release the primary reference on the connection
        // that was taken at creation time
        // nw_release(connection);
      }
      // nw_release(remote);
    });

    nw_connection_start(connection);

    this->send(connection);
    this->receive(connection);
  });

  nw_listener_start(listener);
  return listener;
}

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

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) WKWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@end

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) userContentController: (WKUserContentController *) userContentController
  didReceiveScriptMessage: (WKScriptMessage *) scriptMessage {
    using namespace Opkit;

    id body = [scriptMessage body];
    if (![body isKindOfClass:[NSString class]]) {
      return;
    }

    std::string msg = [body UTF8String];

    if (msg.find("ipc://") == 0) {
      Parse cmd(msg);

      if (cmd.name == "open") {
        // NSString *url = [NSString stringWithUTF8String:cmd.value.c_str()];
        // [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
        return;
      }
    }

    NSLog(@"OPKIT: console '%s'", msg.c_str());
}

- (BOOL) application: (UIApplication *) application
  didFinishLaunchingWithOptions: (NSDictionary *) launchOptions {
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
