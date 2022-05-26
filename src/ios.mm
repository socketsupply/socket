#import <Webkit/Webkit.h>
#include <_types/_uint64_t.h>
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

static dispatch_queue_t queue = dispatch_queue_create("ssc.queue", qos);

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

// Track the state of the network
@property nw_path_monitor_t monitor;
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;

- (void) route: (std::string)msg;

// Bridge to JS
- (void) emit: (std::string)event message: (std::string)message;
- (void) resolve: (std::string)seq message: (std::string)message;
- (void) reject: (std::string)seq message: (std::string)message;

// FS
- (void) fsOpen: (std::string)seq id: (uint64_t)id path: (std::string)path flags: (int)flags;
- (void) fsClose: (std::string)seq id: (uint64_t)id;
- (void) fsRead: (std::string)seq id: (uint64_t)id len: (int)len offset: (int)offset;
- (void) fsWrite: (std::string)seq id: (uint64_t)id data: (std::string)data offset: (int64_t)offset;
- (void) fsStat: (std::string)seq path: (std::string)path;
- (void) fsUnlink: (std::string)seq path: (std::string)path;
- (void) fsRename: (std::string)seq pathA: (std::string)pathA pathB: (std::string)pathB;
- (void) fsCopyFile: (std::string)seq pathA: (std::string)pathA pathB: (std::string)pathB flags: (int)flags;
- (void) fsRmDir: (std::string)seq path: (std::string)path;
- (void) fsMkDir: (std::string)seq path: (std::string)path mode: (int)mode;
- (void) fsReadDir: (std::string)seq path: (std::string)path;

// TCP
- (void) tcpBind: (std::string)seq serverId: (uint64_t)serverId ip: (std::string)ip port: (int)port;
- (void) tcpConnect: (std::string)seq clientId: (uint64_t)clientId port: (int)port ip: (std::string)ip;
- (void) tcpSetTimeout: (std::string)seq clientId: (uint64_t)clientId timeout: (int)timeout;
- (void) tcpSetKeepAlive: (std::string)seq clientId: (uint64_t)clientId timeout: (int)timeout;
- (void) tcpSend: (uint64_t)clientId message: (std::string)message;
- (void) tcpReadStart: (std::string)seq clientId: (uint64_t)clientId;

// UDP
- (void) udpBind: (std::string)seq serverId: (uint64_t)serverId ip: (std::string)ip port: (int)port;
- (void) udpSend: (std::string)seq clientId: (uint64_t)clientId message: (std::string)message offset: (int)offset len: (int)len port: (int)port ip: (const char*)ip;
- (void) udpReadStart: (std::string)seq serverId: (uint64_t)serverId;

// DNS
- (void) dnsLookup: (std::string)seq hostname: (std::string)hostname;

// Shared
- (void) sendBufferSize: (std::string)seq clientId: (uint64_t)clientId size: (int)size;
- (void) recvBufferSize: (std::string)seq clientId: (uint64_t)clientId size: (int)size;
- (void) close: (std::string)seq clientId: (uint64_t)clientId;
- (void) shutdown: (std::string)seq clientId: (uint64_t)clientId;
- (void) readStop: (std::string)seq clientId: (uint64_t)clientId;
@end

@interface IPCSchemeHandler : NSObject<WKURLSchemeHandler>
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask;
@end

std::map<std::string, id<WKURLSchemeTask>> tasks;

@implementation IPCSchemeHandler
- (void)webView: (AppDelegate*)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask {}
- (void)webView: (AppDelegate*)webView startURLSchemeTask:(id <WKURLSchemeTask>)task {
  auto url = std::string(task.request.URL.absoluteString.UTF8String);

  SSC::Parse cmd(url);
  tasks[cmd.get("seq")] = task;

  NSLog(@"BIRP-IPC SCHEME HANDLER");

  [webView route: url];
}
@end

struct GenericContext {
  AppDelegate* delegate;
  uint64_t reqId;
  std::string seq;
};

struct DescriptorContext {
  uv_file fd;
  std::string seq;
  AppDelegate* delegate;
  uint64_t id;
};

struct DirectoryReader {
  uv_dirent_t dirents;
  uv_dir_t* dir;
  uv_fs_t reqOpendir;
  uv_fs_t reqReaddir;
  AppDelegate* delegate;
  std::string seq;
};

struct Peer {
  AppDelegate* delegate;
  std::string seq;

  uv_tcp_t* tcp;
  uv_udp_t* udp;
  uv_stream_t* stream;

  ~Peer () {
    delete this->tcp;
    delete this->udp;
  };
};

struct Server : public Peer {
  uint64_t serverId;
};

struct Client : public Peer {
  Server* server;
  uint64_t clientId;
};

std::string addrToIPv4 (struct sockaddr_in* sin) {
  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
  return std::string(buf);
}

std::string addrToIPv6 (struct sockaddr_in6* sin) {
  char buf[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
  return std::string(buf);
}

struct PeerInfo {
  std::string ip = "";
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
    ip = addrToIPv4((struct sockaddr_in*) &addr);
    port = (int) htons(((struct sockaddr_in*) &addr)->sin_port);
  } else {
    family = "ipv6";
    ip = addrToIPv6((struct sockaddr_in6*) &addr);
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
    ip = addrToIPv4((struct sockaddr_in*) &addr);
    port = (int) htons(((struct sockaddr_in*) &addr)->sin_port);
  } else {
    family = "ipv6";
    ip = addrToIPv6((struct sockaddr_in6*) &addr);
    port = (int) htons(((struct sockaddr_in6*) &addr)->sin6_port);
  }
}

void parse_address (struct sockaddr *name, int* port, char* ip) {
  struct sockaddr_in *name_in = (struct sockaddr_in *) name;
  *port = ntohs(name_in->sin_port);
  uv_ip4_name(name_in, ip, 17);
}

std::map<uint64_t, Client*> clients;
std::map<uint64_t, Server*> servers;
std::map<uint64_t, GenericContext*> contexts;
std::map<uint64_t, DescriptorContext*> descriptors;

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

  // NSLog(@"%s", request.c_str());

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

//
// --- Start filesystem methods
//
- (void) fsOpen: (std::string)seq id: (uint64_t) id path: (std::string)path flags: (int) flags {
  dispatch_async(queue, ^{
    auto desc = descriptors[id] = new DescriptorContext;
    desc->id = id;
    desc->delegate = self;
    desc->seq = seq;

    uv_fs_t req;
    req.data = desc;

    int fd = uv_fs_open(loop, &req, (char*) path.c_str(), flags, 0, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"JSON({
          "data": {
            "fd": $S
          }
        })JSON", std::to_string(desc->id))];

        uv_fs_req_cleanup(req);
      });
    });

    if (fd < 0) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })", std::to_string(id), std::string(uv_strerror(fd)))];
      });
      return;
    }

    desc->fd = fd;
  });
};

- (void) fsClose: (std::string)seq id: (uint64_t)id {
  dispatch_async(queue, ^{
    auto desc = descriptors[id];
    desc->seq = seq;

    if (desc == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"JSON({
          "err": {
            "code": "ENOTOPEN",
            "message": "No file descriptor found with that id"
          }
        })JSON")];
      });
      return;
    }

    uv_fs_t req;
    req.data = desc;

    int err = uv_fs_close(loop, &req, desc->fd, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"JSON({
          "data": {
            "fd": $S
          }
        })JSON", std::to_string(desc->id))];

        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })", std::to_string(id), std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsRead: (std::string)seq id: (uint64_t)id len: (int)len offset: (int)offset {
  dispatch_async(queue, ^{
    auto desc = descriptors[id];

    if (desc == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"JSON({
          "err": {
            "code": "ENOTOPEN",
            "message": "No file descriptor found with that id"
          }
        })JSON")];
      });
      return;
    }

    desc->delegate = self;
    desc->seq = seq;

    uv_fs_t req;
    req.data = desc;

    auto buf = new char[len];
    const uv_buf_t iov = uv_buf_init(buf, (int) len);

    int err = uv_fs_read(loop, &req, desc->fd, &iov, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      if (req->result < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [desc->delegate resolve: desc->seq message: SSC::format(R"JSON({
            "err": {
              "code": "ENOTOPEN",
              "message": "No file descriptor found with that id"
            }
          })JSON")];
        });
        return;
      }

      char *data = req->bufs[0].base;

      NSString* str = [NSString stringWithUTF8String: data];
      NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
      NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

      auto message = std::string([base64Encoded UTF8String]);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i",
          "data": "$S"
        })", std::to_string(desc->id), (int)req->result, message)];

        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })", std::to_string(id), std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsWrite: (std::string)seq id: (uint64_t)id data: (std::string)data offset: (int64_t)offset {
  dispatch_async(queue, ^{
    auto desc = descriptors[id];

    if (desc == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"JSON({
          "err": {
            "code": "ENOTOPEN",
            "message": "No file descriptor found with that id"
          }
        })JSON")];
      });
      return;
    }

    desc->delegate = self;
    desc->seq = seq;

    uv_fs_t req;
    req.data = desc;

    const uv_buf_t buf = uv_buf_init((char*) data.c_str(), (int) data.size());

    int err = uv_fs_write(uv_default_loop(), &req, desc->fd, &buf, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })", std::to_string(id), std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsStat: (std::string)seq path: (std::string)path {
  dispatch_async(queue, ^{
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->delegate = self;
    req.data = desc;

    int err = uv_fs_stat(loop, &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        delete desc;
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsUnlink: (std::string)seq path: (std::string)path {
  dispatch_async(queue, ^{
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->delegate = self;
    req.data = desc;

    int err = uv_fs_unlink(loop, &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        delete desc;
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsRename: (std::string)seq pathA: (std::string)pathA pathB: (std::string)pathB {
  dispatch_async(queue, ^{
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->delegate = self;
    req.data = desc;

    int err = uv_fs_rename(loop, &req, (const char*) pathA.c_str(), (const char*) pathB.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        delete desc;
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsCopyFile: (std::string)seq pathA: (std::string)pathA pathB: (std::string)pathB flags: (int)flags {
  dispatch_async(queue, ^{
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->delegate = self;
    req.data = desc;

    int err = uv_fs_copyfile(loop, &req, (const char*) pathA.c_str(), (const char*) pathB.c_str(), flags, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        delete desc;
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsRmDir: (std::string)seq path: (std::string)path {
  dispatch_async(queue, ^{
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->delegate = self;
    req.data = desc;

    int err = uv_fs_rmdir(loop, &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        delete desc;
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsMkDir: (std::string)seq path: (std::string)path mode: (int)mode {
  dispatch_async(queue, ^{
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->delegate = self;
    req.data = desc;

    int err = uv_fs_mkdir(loop, &req, (const char*) path.c_str(), mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [desc->delegate resolve: desc->seq message: SSC::format(R"({
          "id": "$S",
          "result": "$i"
        })", std::to_string(desc->id), (int)req->result)];

        delete desc;
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

- (void) fsReadDir: (std::string)seq path: (std::string)path {
  dispatch_async(queue, ^{
    DirectoryReader* ctx = new DirectoryReader;
    ctx->delegate = self;
    ctx->seq = seq;

    ctx->reqOpendir.data = ctx;
    ctx->reqReaddir.data = ctx;

    int err = uv_fs_opendir(loop, &ctx->reqOpendir, (const char*) path.c_str(), nullptr);

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
      return;
    }

    err = uv_fs_readdir(loop, &ctx->reqReaddir, ctx->dir, [](uv_fs_t* req) {
      auto ctx = static_cast<DirectoryReader*>(req->data);

      if (req->result < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [ctx->delegate resolve: ctx->seq message: SSC::format(R"({
            "err": {
              "message": "$S"
            }
          })", std::string(uv_strerror((int)req->result)))];
        });
        return;
      }

      std::stringstream value;
      auto len = ctx->dir->nentries;

      for (int i = 0; i < len; i++) {
        value << "\"" << ctx->dir->dirents[i].name << "\"";

        if (i < len - 1) {
          // Assumes the user does not use commas in their file names.
          value << ",";
        }
      }

      NSString* str = [NSString stringWithUTF8String: value.str().c_str()];
      NSData *nsdata = [str dataUsingEncoding: NSUTF8StringEncoding];
      NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

      dispatch_async(dispatch_get_main_queue(), ^{
        [ctx->delegate resolve: ctx->seq message: SSC::format(R"({
          "data": "$S"
        })", std::string([base64Encoded UTF8String]))];
      });

      uv_fs_t reqClosedir;
      uv_fs_closedir(loop, &reqClosedir, ctx->dir, [](uv_fs_t* req) {
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
          "err": {
            "message": "$S"
          }
        })", std::string(uv_strerror(err)))];
      });
    }
  });
};

//
// --- Start network methods
//
- (std::string) getNetworkInterfaces {
  struct ifaddrs *interfaces = nullptr;
  struct ifaddrs *interface = nullptr;
  int success = getifaddrs(&interfaces);
  std::stringstream value;
  std::stringstream v4;
  std::stringstream v6;

  if (success != 0) {
    return "{\"err\": {\"message\":\"unable to get interfaces\"}}";
  }

  interface = interfaces;
  v4 << "\"ipv4\":{";
  v6 << "\"ipv6\":{";

  while (interface != nullptr) {
    std::string ip = "";
    const struct sockaddr_in *addr = (const struct sockaddr_in*)interface->ifa_addr;

    if (addr->sin_family == AF_INET) {
      struct sockaddr_in *addr = (struct sockaddr_in*)interface->ifa_addr;
      v4 << "\"" << interface->ifa_name << "\":\"" << addrToIPv4(addr) << "\",";
    }

    if (addr->sin_family == AF_INET6) {
      struct sockaddr_in6 *addr = (struct sockaddr_in6*)interface->ifa_addr;
      v6 << "\"" << interface->ifa_name << "\":\"" << addrToIPv6(addr) << "\",";
    }

    interface = interface->ifa_next;
  }

  v4 << "\"local\":\"0.0.0.0\"}";
  v6 << "\"local\":\"::1\"}";

  getifaddrs(&interfaces);
  freeifaddrs(interfaces);

  value << "{\"data\":{" << v4.str() << "," << v6.str() << "}}";
  return value.str();
}

- (void) sendBufferSize: (std::string)seq clientId: (uint64_t)clientId size: (int)size {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"JSON({
          "err": {
            "message": "Not connected"
          }
        })JSON")];
      });
      return;
    }

    client->delegate = self;

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    int sz = size;
    int rSize = uv_send_buffer_size(handle, &sz);

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve: seq message: SSC::format(R"JSON({
        "data": {
          "size": $i
        }
      })JSON", rSize)];
    });
    return;
  });
}

- (void) recvBufferSize: (std::string)seq clientId: (uint64_t)clientId size: (int)size {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"JSON({
          "err": {
            "message": "Not connected"
          }
        })JSON")];
      });
      return;
    }

    client->delegate = self;

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    int sz = size;
    int rSize = uv_recv_buffer_size(handle, &sz);

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve: seq message: SSC::format(R"JSON({
        "data": {
          "size": $i
        }
      })JSON", rSize)];
    });
    return;
  });
}

- (void) tcpSend: (uint64_t)clientId message: (std::string)message {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self emit: "error" message: SSC::format(R"JSON({
          "clientId": "$S",
          "data": {
            "message": "Not connected"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    client->delegate = self;

    write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
    req->buf = uv_buf_init((char* const) message.c_str(), (int) message.size());

    auto onWrite = [](uv_write_t *req, int status) {
      auto client = reinterpret_cast<Client*>(req->data);

      if (status) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [client->delegate emit: "error" message: SSC::format(R"({
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

    uv_write((uv_write_t*) req, (uv_stream_t*) client->tcp, &req->buf, 1, onWrite);
  });
}

- (void) tcpConnect: (std::string)seq clientId: (uint64_t)clientId port: (int)port ip: (std::string)ip {
  dispatch_async(queue, ^{
    uv_connect_t connect;

    Client* client = clients[clientId] = new Client();

    client->delegate = self;
    client->clientId = clientId;
    client->tcp = new uv_tcp_t;

    uv_tcp_init(loop, client->tcp);

    client->tcp->data = client;

    uv_tcp_init(loop, client->tcp);
    uv_tcp_nodelay(client->tcp, 0);
    uv_tcp_keepalive(client->tcp, 1, 60);

    struct sockaddr_in dest4;
    struct sockaddr_in6 dest6;

    if (ip.find(":") != std::string::npos) {
      uv_ip6_addr(ip.c_str(), port, &dest6);
    } else {
      uv_ip4_addr(ip.c_str(), port, &dest4);
    }

    // uv_ip4_addr("172.217.16.46", 80, &dest);

    NSLog(@"connect? %s:%i", ip.c_str(), port);

    auto onConnect = [](uv_connect_t* connect, int status) {
      auto* client = reinterpret_cast<Client*>(connect->handle->data);

      NSLog(@"client connection?");

      if (status < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [client->delegate resolve: client->seq message: SSC::format(R"({
            "err": {
              "clientId": "$S",
              "message": "$S"
            }
          })", std::to_string(client->clientId), std::string(uv_strerror(status)))];
        });
        return;
      }

      dispatch_async(dispatch_get_main_queue(), ^{
        [client->delegate resolve: client->seq message: SSC::format(R"({
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
          [client->delegate emit: "data" message: SSC::format(R"({
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

    if (ip.find(":") != std::string::npos) {
      r = uv_tcp_connect(&connect, client->tcp, (const struct sockaddr*) &dest6, onConnect);
    } else {
      r = uv_tcp_connect(&connect, client->tcp, (const struct sockaddr*) &dest4, onConnect);
    }

    if (r) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"({
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

- (void) tcpBind: (std::string) seq serverId: (uint64_t)serverId ip: (std::string)ip port: (int)port {
  dispatch_async(queue, ^{
    loop = uv_default_loop();

    Server* server = servers[serverId] = new Server();
    server->tcp = new uv_tcp_t;
    server->delegate = self;
    server->serverId = serverId;
    server->tcp->data = &server;

    uv_tcp_init(loop, server->tcp);
    struct sockaddr_in addr;

    // addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // NSLog(@"LISTENING %i", addr.sin_addr.s_addr);
    // NSLog(@"LISTENING %s:%i", ip.c_str(), port);

    uv_ip4_addr(ip.c_str(), port, &addr);
    uv_tcp_simultaneous_accepts(server->tcp, 0);
    uv_tcp_bind(server->tcp, (const struct sockaddr*) &addr, 0);

    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, [](uv_stream_t* handle, int status) {
      auto* server = reinterpret_cast<Server*>(handle->data);

      NSLog(@"connection?");

      if (status < 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          [server->delegate emit: "connection" message: SSC::format(R"JSON({
            "serverId": "$S",
            "data": "New connection error $S"
          })JSON", std::to_string(server->serverId), uv_strerror(status))];
        });
        return;
      }

      auto clientId = SSC::rand64();
      Client* client = clients[clientId] = new Client();
      client->clientId = clientId;
      client->server = server;
      client->stream = handle;
      client->tcp = new uv_tcp_t;

      client->tcp->data = client;

      uv_tcp_init(loop, client->tcp);

      if (uv_accept(handle, (uv_stream_t*) handle) == 0) {
        PeerInfo info;
        info.init(client->tcp);

        dispatch_async(dispatch_get_main_queue(), ^{
          [server->delegate
           emit: "connection"
           message: SSC::format(R"JSON({
             "serverId": "$S",
             "clientId": "$S",
             "data": {
               "ip": "$S",
               "family": "$S",
               "port": "$i"
             }
            })JSON",
            std::to_string(server->serverId),
            std::to_string(clientId),
            info.ip,
            info.family,
            info.port
          )];
        });
      } else {
        uv_close((uv_handle_t*) handle, [](uv_handle_t* handle) {
          free(handle);
        });
      }
    });

    if (r) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve: seq message: SSC::format(R"JSON({
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        })JSON", std::to_string(server->serverId), std::string(uv_strerror(r)))];
      });

      NSLog(@"Listener failed: %s", uv_strerror(r));
      return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve: seq message: SSC::format(R"JSON({
        "data": {
          "serverId": "$S",
          "port": "$i",
          "ip": "$S"
        }
      })JSON", std::to_string(server->serverId), port, ip)];
      NSLog(@"Listener started");
    });

    if (isRunning == false) {
      isRunning = true;
      NSLog(@"Starting loop from server");
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

- (void) tcpSetKeepAlive: (std::string)seq clientId: (uint64_t)clientId timeout: (int)timeout {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "No connection found with the specified id"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    client->seq = seq;
    client->delegate = self;
    client->clientId = clientId;

    uv_tcp_keepalive((uv_tcp_t*) client->tcp, 1, timeout);

    dispatch_async(dispatch_get_main_queue(), ^{
      [client->delegate resolve:client->seq message: SSC::format(R"JSON({
        "data": {}
      })JSON")];
    });
  });
}

- (void) tcpSetTimeout: (std::string)seq clientId: (uint64_t)clientId timeout: (int)timeout {
  // TODO
}

- (void) tcpReadStart: (std::string)seq clientId: (uint64_t)clientId {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "No connection found with the specified id"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    client->seq = seq;
    client->delegate = self;

    auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf) {
      auto client = reinterpret_cast<Client*>(handle->data);

      if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, (int) nread);

        NSString* str = [NSString stringWithUTF8String: req->buf.base];
        NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
        NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

        auto serverId = std::to_string(client->server->serverId);
        auto clientId = std::to_string(client->clientId);
        auto message = std::string([base64Encoded UTF8String]);

        dispatch_async(dispatch_get_main_queue(), ^{
          [client->server->delegate emit: "data" message: SSC::format(R"JSON({
            "serverId": "$S",
            "clientId": "$S",
            "data": "$S"
          })JSON", serverId, clientId, message)];
        });
        return;
      }

      if (nread < 0) {
        if (nread != UV_EOF) {
          dispatch_async(dispatch_get_main_queue(), ^{
            [client->server->delegate emit: "error" message: SSC::format(R"JSON({
              "serverId": "$S",
              "data": "$S"
            })JSON", std::to_string(client->server->serverId), uv_err_name((int) nread))];
          });
        }

        uv_close((uv_handle_t*) client->tcp, [](uv_handle_t* handle) {
          free(handle);
        });
      }

      free(buf->base);
    };

    auto allocateBuffer = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
    };

    int err = uv_read_start((uv_stream_t*) client->stream, allocateBuffer, onRead);

    if (err < 0) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        })JSON", std::to_string(client->server->serverId), uv_strerror(err))];
      });
      return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve:client->server->seq message: SSC::format(R"JSON({
        "data": {}
      })JSON")];
    });

    if (isRunning == false) {
      isRunning = true;
      NSLog(@"Starting loop from server");
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

- (void) readStop: (std::string)seq clientId:(uint64_t)clientId {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "No connection with specified id"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    int r;

    if (client->tcp) {
      r = uv_read_stop((uv_stream_t*) client->tcp);
    } else {
      r = uv_read_stop((uv_stream_t*) client->udp);
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [self resolve:client->seq message: SSC::format(R"JSON({
        "data": $i
      })JSON", r)];
    });
  });
}

- (void) close: (std::string)seq clientId:(uint64_t)clientId {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "No connection with specified id"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    client->seq = seq;
    client->delegate = self;
    client->clientId = clientId;

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    handle->data = client;

    uv_close(handle, [](uv_handle_t* handle) {
      auto client = reinterpret_cast<Client*>(handle->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [client->delegate resolve:client->seq message: SSC::format(R"JSON({
          "data": {}
        })JSON")];
      });

      free(handle);
    });
  });
}

- (void) shutdown: (std::string) seq clientId: (uint64_t)clientId {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "No connection with specified id"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    client->seq = seq;
    client->delegate = self;
    client->clientId = clientId;

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    handle->data = client;

    uv_shutdown_t *req = new uv_shutdown_t;
    req->data = handle;

    uv_shutdown(req, (uv_stream_t*) handle, [](uv_shutdown_t *req, int status) {
      auto client = reinterpret_cast<Client*>(req->handle->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [client->delegate resolve:client->seq message: SSC::format(R"JSON({
          "data": {
            "status": "$i"
          }
        })JSON", status)];
      });

      free(req);
      free(req->handle);
    });
  });
}

- (void) udpBind: (std::string)seq serverId: (uint64_t)serverId ip: (std::string)ip port: (int)port {
  dispatch_async(queue, ^{
    loop = uv_default_loop();
    Server* server = servers[serverId] = new Server();
    server->udp = new uv_udp_t;
    server->seq = seq;
    server->serverId = serverId;
    server->delegate = self;
    server->udp->data = server;

    int err;
    struct sockaddr_in addr;

    err = uv_ip4_addr((char *) ip.c_str(), port, &addr);

    if (err < 0) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        })JSON", std::to_string(serverId), uv_strerror(err))];
      });
      return;
    }

    err = uv_udp_bind(server->udp, (const struct sockaddr*) &addr, 0);

    if (err < 0) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [server->delegate emit: "error" message: SSC::format(R"JSON({
          "serverId": "$S",
          "data": "$S"
        })JSON", std::to_string(server->serverId), uv_strerror(err))];
      });
      return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [server->delegate resolve:server->seq message: SSC::format(R"JSON({
        "data": {}
      })JSON")];
    });

    if (isRunning == false) {
      isRunning = true;
      NSLog(@"Starting loop from server");
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

- (void) udpSend: (std::string)seq
         clientId: (uint64_t)clientId
         message: (std::string)message
          offset: (int)offset
             len: (int)len
            port: (int)port
              ip: (const char*)ip {
  dispatch_async(queue, ^{
    Client* client = clients[clientId];

    if (client == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "no such client"
          }
        })JSON", std::to_string(clientId))];
      });
      return;
    }

    client->delegate = self;
    client->seq = seq;

    int err;
    uv_udp_send_t* req = new uv_udp_send_t;
    req->data = client;

    err = uv_ip4_addr((char *) ip, port, &addr);

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "clientId": "$S",
            "message": "$S"
          }
        })JSON", std::to_string(clientId), uv_strerror(err))];
      });
      return;
    }

    uv_buf_t bufs[1];
    char* base = (char*) message.c_str();
    bufs[0] = uv_buf_init(base + offset, len);

    err = uv_udp_send(req, client->udp, bufs, 1, (const struct sockaddr *) &addr, [] (uv_udp_send_t *req, int status) {
      auto client = reinterpret_cast<Client*>(req->data);

      dispatch_async(dispatch_get_main_queue(), ^{
        [client->delegate resolve:client->seq message: SSC::format(R"JSON({
          "data": {
            "clientId": "$S",
            "status": "$i"
          }
        })JSON", std::to_string(client->clientId), status)];
      });

      delete[] req->bufs;
      free(req);
    });

    if (err) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [client->delegate emit: "error" message: SSC::format(R"({
          "clientId": "$S",
          "data": {
            "message": "Write error $S"
          }
        })", std::to_string(client->clientId), uv_strerror(err))];
      });
      return;
    }
  });
}

- (void) udpReadStart: (std::string)seq serverId: (uint64_t)serverId {
  dispatch_async(queue, ^{
    Server* server = servers[serverId];

    if (server == nullptr) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "serverId": "$S",
            "message": "no such client"
          }
        })JSON", std::to_string(serverId))];
      });
      return;
    }

    server->delegate = self;
    server->seq = seq;

    auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
    };

    int err = uv_udp_recv_start(server->udp, allocate, [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
      Server *server = (Server*) handle->data;

      if (nread > 0) {
        int port;
        char ipbuf[17];
        std::string data(buf->base);
        parse_address((struct sockaddr *) addr, &port, ipbuf);
        std::string ip(ipbuf);

        dispatch_async(dispatch_get_main_queue(), ^{
          [server->delegate emit: "data" message: SSC::format(R"JSON({
            "serverId": "$S",
            "port": "$i",
            "ip": "$S",
            "data": "$S"
          })JSON", std::to_string(server->serverId), port, ip, data)];
        });
        return;
      }
    });

    if (err < 0) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self resolve:seq message: SSC::format(R"JSON({
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        })JSON", std::to_string(serverId), uv_strerror(err))];
      });
      return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [server->delegate resolve:server->seq message: SSC::format(R"JSON({
        "data": {}
      })JSON")];
    });
  });
}

- (void) dnsLookup: (std::string)seq hostname: (std::string)hostname {
  dispatch_async(queue, ^{
    loop = uv_default_loop();
    auto reqId = SSC::rand64();
    GenericContext* ctx = contexts[reqId] = new GenericContext;

    struct addrinfo hints;
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    uv_getaddrinfo_t* resolver = new uv_getaddrinfo_t;
    ctx->delegate = self;
    ctx->seq = seq;

    resolver->data = ctx;

    NSLog(@"SETTING UP %s", ctx->seq.c_str());

    uv_getaddrinfo(loop, resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
      auto ctx = (GenericContext*) resolver->data;

      NSLog(@"LOOKING UP %s", ctx->seq.c_str());

      if (status < 0) {
        [ctx->delegate resolve: ctx->seq message: SSC::format(R"JSON({
          "err": {
            "code": "$S",
            "message": "$S"
          }
        })JSON", std::string(uv_err_name((int) status)), std::string(uv_strerror(status)))];

        return;
      }

      char addr[17] = {'\0'};
      uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
      std::string ip(addr, 17);

      NSLog(@"PLEASE RESOLVE %s", ctx->seq.c_str());
      dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"RESOLVING %s", ctx->seq.c_str());
        [ctx->delegate resolve: ctx->seq message: SSC::format(R"JSON({
          "data": "$S"
        })JSON", ip)];
      });

      uv_freeaddrinfo(res);
    }, hostname.c_str(), nullptr, &hints);

    if (isRunning == false) {
      uv_run(loop, UV_RUN_DEFAULT);
    }
  });
}

//
// --- End network methods
//

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
          [self emit: "offline" message: SSC::format(R"JSON({
            "message": "Network path is invalid"
          })JSON")];
        });
        break;
      }
      case nw_path_status_satisfied: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "online" message: SSC::format(R"JSON({
            "message": "Network is usable"
          })JSON")];
        });
        break;
      }
      case nw_path_status_satisfiable: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "online" message: SSC::format(R"JSON({
            "message": "Network may be usable"
          })JSON")];
        });
        break;
      }
      case nw_path_status_unsatisfied: {
        dispatch_async(dispatch_get_main_queue(), ^{
          [self emit: "offline" message: SSC::format(R"JSON({
            "message": "Network is not usable"
          })JSON")];
        });
        break;
      }
    }
  });

  nw_path_monitor_start(self.monitor);
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

    NSLog(@"BIRP-IPC UCC");

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

      "const consoleOld = console.log;\n"
      "console.log = (...args) => {\n"
      "  consoleOld(...args);\n"
      "  window.external.invoke(JSON.stringify(args));\n"
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

    [config setURLSchemeHandler:[IPCSchemeHandler new] forURLScheme:@"ipc"];

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

    // [self.webview evaluateJavaScript: @"window.bar = 2 + 2;" completionHandler:^(NSString *result, NSError *error)
    // {
    //     NSLog(@"Error %@",error);
    //       NSLog(@"Result %@",result);
    // }];

    return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}

