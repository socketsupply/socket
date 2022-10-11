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

#include "../common.hh"

@class SSCIPCBridge;

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

@interface SSCBluetoothDelegate : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate>
@property (strong, nonatomic) SSCIPCBridge* bridge;
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

#endif
