#ifndef SSC_CORE_APPLE_H
#define SSC_CORE_APPLE_H

#import <Webkit/Webkit.h>
#import <Network/Network.h>
#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import <UserNotifications/UserNotifications.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#include "common.hh"

namespace SSC {
  class Core;
  struct Post;
  using Task = id<WKURLSchemeTask>;
  using Tasks = std::map<String, Task>;
}

@class Bridge;

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("co.socketsupply.queue.core", qos);

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface SSCBridgedWebView : WKWebView
@end
#else
@interface SSCBridgedWebView : WKWebView<
  WKUIDelegate,
  NSDraggingDestination,
  NSFilePromiseProviderDelegate,
  NSDraggingSource>
- (NSDragOperation) draggingSession: (NSDraggingSession *) session
sourceOperationMaskForDraggingContext: (NSDraggingContext) context;
@end
#endif

@interface SSCNavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webview: (SSCBridgedWebView*) webview
  decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end

@interface SSCIPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (strong, nonatomic) Bridge* bridge;
- (void) webView: (SSCBridgedWebView*) webview
  startURLSchemeTask: (id <WKURLSchemeTask> )urlSchemeTask;
- (void) webView: (SSCBridgedWebView* )webview
  stopURLSchemeTask: (id <WKURLSchemeTask>) urlSchemeTask;
@end

@interface SSCBluetoothDelegate : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate>
@property (strong, nonatomic) Bridge* bridge;
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) NSMutableDictionary* services;
@property (strong, nonatomic) NSMutableDictionary* characteristics;
@property (strong, nonatomic) NSMutableDictionary* serviceMap;
- (void) publishCharacteristic: (SSC::String) seq
                           buf: (char*) buf
                           len: (int) len
                           sid: (SSC::String) sid
                           cid: (SSC::String) cid;
- (void) subscribeCharacteristic: (SSC::String)
                         seq sid: (SSC::String)
                         sid cid: (SSC::String) cid;
- (void) startService: (SSC::String)
              seq sid: (SSC::String) sid;
- (void) startAdvertising;
- (void) startScanning;
- (void) initBluetooth;
@end

@interface Bridge : NSObject {
  std::unique_ptr<SSC::Tasks> tasks;
  std::recursive_mutex tasksMutex;
}
@property (strong, nonatomic) SSCBluetoothDelegate* bluetooth;
@property (strong, nonatomic) SSCBridgedWebView* webview;
@property (nonatomic) SSC::Core* core;
@property nw_path_monitor_t monitor;
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
- (bool) route: (SSC::String) msg
           buf: (char*) buf
       bufsize: (size_t) bufsize;
- (void) emit: (SSC::String) name
          msg: (SSC::String) msg;
- (void) send: (SSC::String) seq
          msg: (SSC::String) msg
         post: (SSC::Post) post;
- (void) initNetworkStatusObserver;
- (void) setBluetooth: (SSCBluetoothDelegate*) bd;
- (void) setWebview: (SSCBridgedWebView*) bv;
- (void) setCore: (SSC::Core*)core;
- (SSC::Task) getTask: (SSC::String) id;
- (bool) hasTask: (SSC::String) id;
- (void) removeTask: (SSC::String) id;
- (void) putTask: (SSC::String) id
            task: (SSC::Task) task;
@end

#endif
