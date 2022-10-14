#include "ipc.hh"

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("co.socketsupply.queue.bridge", qos);

namespace SSC::IPC {
  using Task = id<WKURLSchemeTask>;
  using Tasks = std::map<String, Task>;
}

@interface SSCIPCBridge () {
  std::unique_ptr<SSC::IPC::Tasks> tasks;
  std::recursive_mutex tasksMutex;
}

- (SSC::IPC::Task) getTask: (SSC::String) id;
- (void) removeTask: (SSC::String) id;
- (bool) hasTask: (SSC::String) id;
- (void) putTask: (SSC::String) id
            task: (SSC::IPC::Task) task;
@end

@implementation SSCIPCBridgeSchemeHandler
- (void) webView: (SSCBridgedWebView*) webview stopURLSchemeTask: (SSC::IPC::Task) task {}
- (void) webView: (SSCBridgedWebView*) webview startURLSchemeTask: (SSC::IPC::Task) task {
  auto url = SSC::String(task.request.URL.absoluteString.UTF8String);

  SSC::IPC::Message message(url);

  if (SSC::String(task.request.HTTPMethod.UTF8String) == "OPTIONS") {
    NSMutableDictionary* httpHeaders = [NSMutableDictionary dictionary];

    httpHeaders[@"access-control-allow-origin"] = @"*";
    httpHeaders[@"access-control-allow-methods"] = @"*";

    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: httpHeaders
    ];

    [task didReceiveResponse: httpResponse];
    [task didFinish];
    #if !__has_feature(objc_arc)
    [httpResponse release];
    #endif

    return;
  }

  if (message.name == "post" || message.name == "data") {
    NSMutableDictionary* httpHeaders = [NSMutableDictionary dictionary];
    uint64_t postId = std::stoull(message.get("id"));
    auto post = self.bridge.core->getPost(postId);

    httpHeaders[@"access-control-allow-origin"] = @"*";
    httpHeaders[@"content-length"] = [@(post.length) stringValue];

    if (post.headers.size() > 0) {
      auto lines = SSC::split(SSC::trim(post.headers), '\n');

      for (auto& line : lines) {
        auto pair = SSC::split(SSC::trim(line), ':');
        NSString* key = [NSString stringWithUTF8String: SSC::trim(pair[0]).c_str()];
        NSString* value = [NSString stringWithUTF8String: SSC::trim(pair[1]).c_str()];
        httpHeaders[key] = value;
      }
    }

    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
       initWithURL: task.request.URL
        statusCode: 200
       HTTPVersion: @"HTTP/1.1"
      headerFields: httpHeaders
    ];

    [task didReceiveResponse: httpResponse];

    if (post.body) {
      NSData *data = [NSData dataWithBytes: post.body length: post.length];
      [task didReceiveData: data];
    } else {
      NSString* str = [NSString stringWithUTF8String: ""];
      NSData* data = [str dataUsingEncoding: NSUTF8StringEncoding];
      [task didReceiveData: data];
    }

    [task didFinish];
    #if !__has_feature(objc_arc)
    [httpResponse release];
    #endif

    // 16ms timeout before removing post and potentially freeing `post.body`
    NSTimeInterval timeout = 0.16;
    auto block = ^(NSTimer* timer) {
      dispatch_async(dispatch_get_main_queue(), ^{
        self.bridge.core->removePost(postId);
      });
    };

    [NSTimer timerWithTimeInterval: timeout repeats: NO block: block ];

    return;
  }

  size_t bufsize = 0;
  char *body = NULL;
  auto seq = message.get("seq");

  if (seq.size() > 0 && seq != "-1") {
    #if !__has_feature(objc_arc)
    [task retain];
    #endif

    [self.bridge putTask: seq task: task];
  }

  // if there is a body on the reuqest, pass it into the method router.
  auto rawBody = task.request.HTTPBody;

  if (rawBody) {
    const void* data = [rawBody bytes];
    bufsize = [rawBody length];
    body = (char*)data;
  }

  if (![self.bridge route: url buf: body bufsize: bufsize]) {
    NSMutableDictionary* httpHeaders = [NSMutableDictionary dictionary];
    auto msg = SSC::format(R"MSG({
      "err": {
        "message": "Not found",
        "type": "NotFoundError",
        "url": "$S"
      }
    })MSG", url);

    auto str = [NSString stringWithUTF8String: msg.c_str()];
    auto data = [str dataUsingEncoding: NSUTF8StringEncoding];

    httpHeaders[@"access-control-allow-origin"] = @"*";
    httpHeaders[@"content-length"] = [@(msg.size()) stringValue];

    NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
       initWithURL: task.request.URL
        statusCode: 404
       HTTPVersion: @"HTTP/1.1"
      headerFields: httpHeaders
    ];

    [task didReceiveResponse: httpResponse];
    [task didReceiveData: data];
    [task didFinish];
    #if !__has_feature(objc_arc)
    [httpResponse release];
    #endif
  }
}
@end

@implementation SSCIPCBridge
- (id) init {
  self = [super init];
  tasks = std::unique_ptr<SSC::IPC::Tasks>(new SSC::IPC::Tasks());
  return self;
}

- (SSC::IPC::Task) getTask: (SSC::String) id {
  std::lock_guard<std::recursive_mutex> guard(tasksMutex);
  if (tasks->find(id) == tasks->end()) return SSC::IPC::Task{};
  return tasks->at(id);
}

- (bool) hasTask: (SSC::String) id {
  std::lock_guard<std::recursive_mutex> guard(tasksMutex);
  if (id.size() == 0) return false;
  return tasks->find(id) != tasks->end();
}

- (void) removeTask: (SSC::String) id {
  std::lock_guard<std::recursive_mutex> guard(tasksMutex);
  if (tasks->find(id) == tasks->end()) return;
  tasks->erase(id);
}

- (void) putTask: (SSC::String) id task: (SSC::IPC::Task) task {
  std::lock_guard<std::recursive_mutex> guard(tasksMutex);
  tasks->insert_or_assign(id, task);
}

- (void) dispatch: (void (^)(void)) callback {
  dispatch_async(queue, ^{
    callback();
  });
}

- (void) setBluetooth: (SSCBluetoothDelegate*)bd {
  _bluetooth = bd;
  [_bluetooth initBluetooth];
  _bluetooth.bridge = self;
}

- (void) initNetworkStatusObserver {
  dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_UTILITY,
    DISPATCH_QUEUE_PRIORITY_DEFAULT
  );

  self.monitorQueue = dispatch_queue_create("co.socketsupply.queue.network-monitor", attrs);

  // self.monitor = nw_path_monitor_create_with_type(nw_interface_type_wifi);
  self.monitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(self.monitor, self.monitorQueue);
  nw_path_monitor_set_update_handler(self.monitor, ^(nw_path_t path) {
    nw_path_status_t status = nw_path_get_status(path);

    SSC::String name;
    SSC::String message;

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

    dispatch_async(dispatch_get_main_queue(), ^{
      [self emit: name msg: SSC::format(R"JSON({
        "message": "$S"
      })JSON", message)];
    });
  });

  nw_path_monitor_start(self.monitor);
}

- (void) setWebview: (SSCBridgedWebView*)wv {
  _webview = wv;
}

- (void) setCore: (SSC::Core*)core; {
  _core = core;
}

- (void) emit: (SSC::String) name
          msg: (SSC::String) msg
{
  self.router->emit(name, msg);
}

- (void) send: (SSC::IPC::Message::Seq) seq
          msg: (SSC::String) msg
         post: (SSC::Post) post
{
  if (seq != "-1" && [self hasTask: seq]) {
    auto task = [self getTask: seq];
    [self removeTask: seq];

    #if !__has_feature(objc_arc)
    [task retain];
    #endif

    dispatch_async(dispatch_get_main_queue(), ^{
      NSMutableDictionary* httpHeaders = [[NSMutableDictionary alloc] init];
      auto length = post.body ? post.length : msg.size();

      httpHeaders[@"access-control-allow-origin"] = @"*";
      httpHeaders[@"access-control-allow-methods"] = @"*";
      httpHeaders[@"content-length"] = [@(length) stringValue];

      NSHTTPURLResponse *httpResponse = [[NSHTTPURLResponse alloc]
         initWithURL: task.request.URL
          statusCode: 200
         HTTPVersion: @"HTTP/1.1"
        headerFields: httpHeaders
      ];

      [task didReceiveResponse: httpResponse];

      if (post.body) {
        NSData *data = [NSData dataWithBytes: post.body length: post.length];
        [task didReceiveData: data];
      } else if (msg.size() > 0) {
        NSString* str = [NSString stringWithUTF8String: msg.c_str()];
        NSData *data = [str dataUsingEncoding: NSUTF8StringEncoding];
        [task didReceiveData: data];
      }

      [task didFinish];
      #if !__has_feature(objc_arc)
      [httpHeaders release];
      [httpResponse release];
      #endif
    });
    return;
  }

  self.router->send(seq, msg, post);
}

- (void) userNotificationCenter: (UNUserNotificationCenter *) center
        willPresentNotification: (UNNotification *) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler
{
  completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}

// returns true if routable (regardless of success)
- (bool) route: (SSC::String) msg buf: (char*) buf bufsize: (size_t) bufsize {
  using namespace SSC;

  if (msg.find("ipc://") != 0) return false;

  auto invoked = self.router->invoke(msg, buf, bufsize, [=](auto result) {
    [self send: seq msg: msg post: post];
  });

  if (invoked)
    return true;
  }

  IPC::Message message(msg);
  auto seq = message.get("seq");
  // NSLog(@"Route<%s> - [%s:%i]", message.name.c_str(), buf, (int)bufsize);

  uint64_t peerId = 0;

  /// ipc bluetooth-start
  /// @param serviceId String
  ///
  if (message.name == "bluetooth.start") {
    dispatch_async(queue, ^{
      [self.bluetooth startService: seq sid: message.get("serviceId")];
    });
    return true;
  }

  if (message.name == "bluetooth.subscribe") {
    auto cid = message.get("characteristicId");
    auto sid = message.get("serviceId");
    auto seq = message.get("seq");

    dispatch_async(queue, ^{
      [self.bluetooth subscribeCharacteristic: seq sid: sid cid: cid];
    });
    return true;
  }

  if (message.name == "bluetooth.publish") {
    auto sid = message.get("serviceId");
    auto cid = message.get("characteristicId");
    auto seq = message.get("seq");

    if (sid.size() != 36) {
      auto msg = SSC::format(R"MSG({ "err": { "message": "invalid serviceId" } })MSG");

      dispatch_async(queue, ^{
        [self send: seq msg: msg post: Post{}];
      });
      return true;
    }

    if (sid.size() != 36) {
      auto msg = SSC::format(R"MSG({ "err": { "message": "invalid characteristicId" } })MSG");
      dispatch_async(queue, ^{
        [self send: seq msg: msg post: Post{}];
      });
      return true;
    }

    char* value;
    int len;

    if (buf != nullptr) {
      len = std::stoi(message.get("length"));
      value = buf;
    } else {
      value = message.get("value").data();
      len = (int) message.get("value").size();
    }

    [self.bluetooth publishCharacteristic: seq buf: value len: len sid: sid cid: cid];
    return true;
  }

  if (message.name == "notify") {
    UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
    content.body = [NSString stringWithUTF8String: message.get("body").c_str()];
    content.title = [NSString stringWithUTF8String: message.get("title").c_str()];
    content.sound = [UNNotificationSound defaultSound];

    UNTimeIntervalNotificationTrigger* trigger = [
      UNTimeIntervalNotificationTrigger triggerWithTimeInterval: 1.0f repeats: NO
    ];

    UNNotificationRequest *request = [UNNotificationRequest
      requestWithIdentifier: @"LocalNotification" content: content trigger: trigger
    ];

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

    [center requestAuthorizationWithOptions: (UNAuthorizationOptionSound | UNAuthorizationOptionAlert | UNAuthorizationOptionBadge) completionHandler:^(BOOL granted, NSError* error) {
      if (error) {
        [center addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
          if (error) debug("Unable to create notification: %@", error.debugDescription);
        }];
      }
    }];
    return true;
  }

  if (message.name == "log") {
    NSLog(@"%s\n", message.value.c_str());
    return true;
  }

  if (message.name == "window.eval") {
    SSC::String value = decodeURIComponent(message.get("value"));
    auto seq = message.get("seq");

    NSString* script = [NSString stringWithUTF8String: value.c_str()];
    dispatch_async(dispatch_get_main_queue(), ^{
      [self.webview evaluateJavaScript: script completionHandler: ^(id result, NSError *error) {
        if (result) {
          auto msg = SSC::String([[NSString stringWithFormat:@"%@", result] UTF8String]);
          [self send: seq msg: msg post: Post{}];
        } else if (error) {
          auto exception = error.userInfo[@"WKJavaScriptExceptionMessage"];
          auto message = [[NSString stringWithFormat:@"%@", exception] UTF8String];
          auto err = encodeURIComponent(SSC::String(message));

          if (err == "(null)") {
            [self send: seq msg: "null" post: Post{}];
            return;
          }

          auto msg = SSC::format(
            R"MSG({"err": { "message": "Error: $S" } })MSG",
            err
          );

          [self send: seq msg: msg post: Post{}];
        } else {
          [self send: seq msg: "undefined" post: Post{}];
        }
      }];
    });

    return true;
  }

  if (message.name == "fs.constants") {
    dispatch_async(queue, ^{
      auto constants = self.core->getFSConstants();
      [self send: seq msg: constants post: Post{}];
    });
    return true;
  }

  if (message.name == "fs.getOpenDescriptors") {
    dispatch_async(queue, ^{
      self.core->fsGetOpenDescriptors(seq, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.retainOpenDescriptor") {
    auto id = std::stoull(message.get("id"));

    dispatch_async(queue, ^{
      self.core->fsRetainOpenDescriptor(seq, id, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.rmdir") {
    auto path = decodeURIComponent(message.get("path"));

    dispatch_async(queue, ^{
      self.core->fsRmdir(seq, path, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.access") {
    auto path = decodeURIComponent(message.get("path"));
    auto mode = std::stoi(message.get("mode"));

    dispatch_async(queue, ^{
      self.core->fsAccess(seq, path, mode, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.open") {
    auto cid = std::stoull(message.get("id"));
    auto path = decodeURIComponent(message.get("path"));
    auto flags = std::stoi(message.get("flags"));
    auto mode = std::stoi(message.get("mode"));

    dispatch_async(queue, ^{
      self.core->fsOpen(seq, cid, path, flags, mode, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.close") {
    auto id = std::stoull(message.get("id"));

    dispatch_async(queue, ^{
      self.core->fsClose(seq, id, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.closeOpenDescriptor") {
    auto id = std::stoull(message.get("id"));

    dispatch_async(queue, ^{
      self.core->fsCloseOpenDescriptor(seq, id, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.closeOpenDescriptors") {
    auto preserveRetained = message.get("retain") != "false";
    dispatch_async(queue, ^{
      self.core->fsCloseOpenDescriptors(seq, preserveRetained, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.read") {
    auto id = std::stoull(message.get("id"));
    auto size = std::stoi(message.get("size"));
    auto offset = std::stoi(message.get("offset"));

    dispatch_async(queue, ^{
      self.core->fsRead(seq, id, size, offset, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.write") {
    auto id = std::stoull(message.get("id"));
    auto offset = std::stoull(message.get("offset"));
    auto data = SSC::String(buf, bufsize);

    dispatch_async(queue, ^{
      self.core->fsWrite(seq, id, data, offset, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.stat") {
    auto path = decodeURIComponent(message.get("path"));

    dispatch_async(queue, ^{
      self.core->fsStat(seq, path, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.fstat") {
    auto id = std::stoull(message.get("id"));

    dispatch_async(queue, ^{
      self.core->fsFStat(seq, id, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.unlink") {
    auto path = decodeURIComponent(message.get("path"));

    dispatch_async(queue, ^{
      self.core->fsUnlink(seq, path, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.rename") {
    auto src = decodeURIComponent(message.get("src"));
    auto dst = decodeURIComponent(message.get("dst"));

    dispatch_async(queue, ^{
      self.core->fsRename(seq, src, dst, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.copyFile") {
    auto flags = std::stoi(message.get("flags", "0"));
    auto src = decodeURIComponent(message.get("src"));
    auto dst = decodeURIComponent(message.get("dst"));

    dispatch_async(queue, ^{
      self.core->fsCopyFile(seq, src, dst, flags, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.mkdir") {
    auto path = decodeURIComponent(message.get("path"));
    auto mode = std::stoi(message.get("mode"));

    dispatch_async(queue, ^{
      self.core->fsMkdir(seq, path, mode, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.opendir") {
    auto id = std::stoull(message.get("id"));
    auto path = decodeURIComponent(message.get("path"));

    dispatch_async(queue, ^{
      self.core->fsOpendir(seq, id, path, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.readdir") {
    auto id = std::stoull(message.get("id"));
    auto entries = std::stoi(message.get("entries", "256"));

    dispatch_async(queue, ^{
      self.core->fsReaddir(seq, id, entries, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "fs.closedir") {
    auto id = std::stoull(message.get("id"));

    dispatch_async(queue, ^{
      self.core->fsClosedir(seq, id, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  // TODO this is a generalization that doesnt work
  if (message.get("id").size() != 0) {
    try {
      peerId = std::stoull(message.get("id"));
    } catch (...) {
      auto msg = SSC::format(R"MSG({
        "err": { "message": "invalid peerId" }
      })MSG");
      [self send: seq msg: msg post: Post{}];
      return true;
    }
  }

  if (message.name == "external" || message.name == "open.external") {
    NSString *url = [NSString stringWithUTF8String:SSC::decodeURIComponent(message.get("value")).c_str()];
    #if MACOS == 1
      [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: url]];
    #else
      [[UIApplication sharedApplication] openURL: [NSURL URLWithString:url] options: @{} completionHandler: nil];
    #endif
    return true;
  }

  if (message.name == "os.networkInterfaces") {
    dispatch_async(queue, ^{
      auto msg = self.core->getNetworkInterfaces();
      [self send: seq msg: msg post: Post{}];
    });
    return true;
  }

  if (message.name == "cwd" || message.name == "process.cwd") {
    NSFileManager *fileManager;
    NSString *currentDirectoryPath;

    fileManager = [NSFileManager defaultManager];
    currentDirectoryPath = [fileManager currentDirectoryPath];
    NSString *cwd = [NSHomeDirectory() stringByAppendingPathComponent: currentDirectoryPath];

    dispatch_async(queue, ^{
      auto msg = SSC::format(R"JSON({
        "source": "process.cwd",
        "data": "$S"
      })JSON", SSC::String([cwd UTF8String]));
      [self send: seq msg: msg post: Post{}];
    });
    return true;
  }

  if (message.name == "os.platform") {
    dispatch_async(queue, ^{
      auto msg = SSC::format(R"JSON({
        "source": "os.platform",
        "data": "$S"
      })JSON", SSC::platform.os);

      [self send: seq msg: msg post: Post{}];
    });
    return true;
  }

  if (message.name == "os.type") {
    dispatch_async(queue, ^{
      auto msg = SSC::format(R"JSON({
        "source": "os.type",
        "data": "$S"
      })JSON", SSC::platform.os);

      [self send: seq msg: msg post: Post{}];
    });
    return true;
  }

  if (message.name == "os.arch") {
    dispatch_async(queue, ^{
      auto msg = SSC::format(R"JSON({
        "source": "os.arch",
        "data": "$S"
      })JSON", SSC::platform.arch);

      [self send: seq msg: msg post: Post{}];
    });
    return true;
  }

  if (message.name == "bufferSize") {
    auto buffer = std::stoi(message.get("buffer", "0"));
    auto size = std::stoi(message.get("size", "0"));
    uint64_t id = 0ll;

    try {
      id = std::stoull(message.get("id"));
    } catch (...) {
      dispatch_async(queue, ^{
        auto err = SSC::format(R"MSG({
          "source": "bufferSize",
          "err": {
            "type": "InternalError",
            "message": ".id is required"
          }
        })MSG");

        [self send: seq msg: err post: Post{}];
      });
      return true;
    }

    dispatch_async(queue, ^{
      self.core->bufferSize(seq, id, size, buffer, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.close" || message.name == "close") {
    auto peerId = std::stoull(message.get("id"));

    dispatch_async(queue, ^{
      self.core->close(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.connect") {
    auto strId = message.get("id");
    SSC::String err = "";
    uint64_t peerId = 0ll;
    int port = 0;
    auto strPort = message.get("port");
    auto ip = message.get("address");

    if (strId.size() == 0) {
      err = "invalid peerId";
    } else {
      try {
        peerId = std::stoull(message.get("id"));
      } catch (...) {
        err = "invalid peerId";
      }
    }

    if (strPort.size() == 0) {
      err = "invalid port";
    } else {
      try {
        port = std::stoi(strPort);
      } catch (...) {
        err = "invalid port";
      }
    }

    if (port == 0) {
      err = "Can not bind to port 0";
    }

    if (err.size() > 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.connect",
        "err": {
          "message": "$S"
        }
      })MSG", err);
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    dispatch_async(queue, ^{
      self.core->udpConnect(seq, peerId, ip, port, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.disconnect") {
    auto strId = message.get("id");

    if (strId.size() == 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.disconnect",
        "err": {
          "message": "expected .peerId"
        }
      })MSG");

      dispatch_async(queue, ^{
        [self send: seq msg: msg post: Post{}];
      });
      return true;
    }

    auto peerId = std::stoull(strId);

    dispatch_async(queue, ^{
      self.core->udpDisconnect(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.getPeerName") {
    auto strId = message.get("id");

    if (strId.size() == 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getPeerName",
        "err": {
          "message": "expected .peerId"
        }
      })MSG");

      dispatch_async(queue, ^{
        [self send: seq msg: msg post: Post{}];
      });
      return true;
    }

    auto peerId = std::stoull(strId);

    dispatch_async(queue, ^{
      self.core->udpGetPeerName(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.getSockName") {
    auto strId = message.get("id");

    if (strId.size() == 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getSockName",
        "err": {
          "message": "expected either .id"
        }
      })MSG");

      dispatch_async(queue, ^{
        [self send: seq msg: msg post: Post{}];
      });
      return true;
    }

    auto peerId = std::stoull(strId);

    dispatch_async(queue, ^{
      self.core->udpGetSockName(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.getState") {
    auto strId = message.get("id");

    if (strId.size() == 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getState",
        "err": {
          "message": "expected either .id"
        }
      })MSG");

      dispatch_async(queue, ^{
        [self send: seq msg: msg post: Post{}];
      });
      return true;
    }

    auto peerId = std::stoull(strId);

    dispatch_async(queue, ^{
      self.core->udpGetState(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  if (message.name == "udp.send") {
    int offset = 0;
    int port = 0;
    uint64_t peerId;
    SSC::String err;

    auto ephemeral = message.get("ephemeral") == "true";
    auto strOffset = message.get("offset");
    auto strPort = message.get("port");
    auto ip = message.get("address");

    if (strOffset.size() > 0) {
      try {
        offset = std::stoi(strOffset);
      } catch (...) {
        err = "invalid offset";
      }
    }

    try {
      port = std::stoi(strPort);
    } catch (...) {
      err = "invalid port";
    }

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    try {
      peerId = std::stoull(message.get("id"));
    } catch (...) {
      err = "invalid peerId";
    }

    if (bufsize <= 0) {
      err = "invalid buffer size";
    }

    if (err.size() > 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.send",
        "err": {
          "message": "$S"
        }
      })MSG", err);
      [self send: seq msg: err post: Post{}];
      return true;
    }

    auto tmp = new char[bufsize]{0};
    memcpy(tmp, buf, bufsize);
    dispatch_async(queue, ^{
      self.core->udpSend(seq, peerId, tmp, (int)bufsize, port, ip, ephemeral, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
        delete [] tmp;
      });
    });
    return true;
  }

  if (message.name == "udp.bind") {
    auto ip = message.get("address");
    auto reuseAddr = message.get("reuseAddr") == "true";
    SSC::String err;
    int port;
    uint64_t peerId = 0ll;

    if (ip.size() == 0) {
      ip = "0.0.0.0";
    }

    try {
      peerId = std::stoull(message.get("id"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'peerId' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    try {
      port = std::stoi(message.get("port"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'port' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    dispatch_async(queue, ^{
      self.core->udpBind(seq, peerId, ip, port, reuseAddr, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });

    return true;
  }

  if (message.name == "udp.readStart") {
    uint64_t peerId;

    try {
      peerId = std::stoull(message.get("id"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'peerId' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    dispatch_async(queue, ^{
      self.core->udpReadStart(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });

    return true;
  }

  if (message.name == "udp.readStop") {
    uint64_t peerId;

    try {
      peerId = std::stoull(message.get("id"));
    } catch (...) {
      auto msg = SSC::format(R"({ "err": { "message": "property 'peerId' required" } })");
      [self send: seq msg: msg post: Post{}];
      return true;
    }

    dispatch_async(queue, ^{
      self.core->udpReadStop(seq, peerId, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });

    return true;
  }

  if (message.name == "dns.lookup") {
    auto hostname = message.get("hostname");
    SSC::String err = "";

    auto strFamily = message.get("family");
    int family = 0;

    if (strFamily.size() > 0) {
      try {
        family = std::stoi(strFamily);
      } catch (...) {
      }
    }

    // TODO: support these options
    // auto family = std::stoi(message.get("family"));
    // auto hints = std::stoi(message.get("hints"));
    // auto all = bool(std::stoi(message.get("all")));
    // auto verbatim = bool(std::stoi(message.get("verbatim")));

    dispatch_async(queue, ^{
      self.core->dnsLookup(seq, hostname, family, [=](auto seq, auto msg, auto post) {
        [self send: seq msg: msg post: post];
      });
    });
    return true;
  }

  return false;
}
@end
