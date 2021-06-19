#include "common.hh"
#import <Cocoa/Cocoa.h>
#import <Webkit/Webkit.h>
#include <objc/objc-runtime.h>

// ObjC declarations may only appear in global scope
@interface WindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
@end

@implementation WindowDelegate
- (void)userContentController:(WKUserContentController*)userContentController
      didReceiveScriptMessage:(WKScriptMessage*)scriptMessage {
}
@end

namespace Opkit { 

  class App: public IApp {
    NSAutoreleasePool* pool = [NSAutoreleasePool new];

    public:
      int run();
      void exit();
      void dispatch(std::function<void()> work);
      String getCwd(String);
  };

  class Window : public IWindow {
    NSWindow* window;
    WKWebView* webview;

    public:
      App app;
      Window(App&, WindowOptions);

      void onMessage(std::function<void(String)>);
      void eval(const String&);
      void show();
      void hide();
      void navigate(const String&);
      void setTitle(const String&);
      void setContextMenu(String, String);
      String openDialog(bool, bool, bool, String, String);

      void setSystemMenu(String menu);
      int openExternal(String s);
  };

  int App::run () {
    /* NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:true];
    if (event) {
        [NSApp sendEvent:event];
    } */

    [NSApp run];
    return shouldExit;
  }

  // We need to dispatch any code that touches
  // a window back into the main thread.
  void App::dispatch(std::function<void()> f) {
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        f();
      });
    });
  }

  void App::exit () {
    // Distinguish window closing with app exiting
    shouldExit = true;
    [NSApp terminate:nil];
  }

  Window::Window (App& app, WindowOptions opts) {
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled;

    // Set window to be resizable
    if (opts.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (opts.frameless) {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
    } else {
      style |= NSWindowStyleMaskClosable;
      style |= NSWindowStyleMaskMiniaturizable;
    }

    // Initialize Cocoa window
    window = [[NSWindow alloc]
        // Initial window size
        initWithContentRect:NSMakeRect(0, 0, opts.width, opts.height)
                  // Window style
                  styleMask:style
                    backing:NSBackingStoreBuffered
                      defer:NO];

    // Minimum window size
    [window setContentMinSize:NSMakeSize(opts.width, opts.height)];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent:true];
    }

    // Position window in center of screen
    [window center];

    // Initialize WKWebView
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    WKPreferences* prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    WKUserContentController* controller = [config userContentController];

    // Add preload script, normalizing the interface to be cross-platform.
    String preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
      "};\n"
      "" + opts.preload + "\n"
    );

    WKUserScript* userScript = [WKUserScript alloc];
    [userScript initWithSource:[NSString stringWithUTF8String:preload.c_str()]
                 injectionTime:WKUserScriptInjectionTimeAtDocumentStart
              forMainFrameOnly:NO];
    [controller addUserScript:userScript];

    webview = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];


    [webview.configuration.preferences
      setValue:@YES
        forKey:@"allowFileAccessFromFileURLs"];

    // Add delegate methods manually in order to capture "this"
    class_replaceMethod(
      [WindowDelegate class], @selector(windowWillClose:),
      imp_implementationWithBlock(
        [&](id self, SEL cmd, id notification) {
          app.exit();
        }),
      "v@:@"
    );

    class_replaceMethod(
      [WindowDelegate class],
      @selector(userContentController:didReceiveScriptMessage:),
      imp_implementationWithBlock(
        [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
          if (this->_onMessage == nullptr) return;

          id body = [scriptMessage body];
          if (![body isKindOfClass:[NSString class]]) {
            return;
          }

          String msg = [body UTF8String];
          this->_onMessage(msg);
        }),
      "v@:@"
    );

    class_addMethod(
      [WindowDelegate class],
      @selector(menuItemSelected:),
      imp_implementationWithBlock(
        [=](id self, SEL _cmd, id item) {
          id menuItem = (id) item;
          String title = [[menuItem title] UTF8String];
          String state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
          String parent = [[[menuItem menu] title] UTF8String];
          String seq = std::to_string([menuItem tag]);

          eval("(() => {"
            "  const detail = {"
            "    title: '" + title + "',"
            "    parent: '" + parent + "',"
            "    state: '" + state + "'"
            "  };"

            "  if (" + seq + " > 0) {"
            "    window._ipc[" + seq + "].resolve(detail);"
            "    delete window._ipc[" + seq + "];"
            "    return;"
            "  }"

            "  const event = new window.CustomEvent('menuItemSelected', { detail });"
            "  window.dispatchEvent(event);"
            "})()"
          );
        }),
      "v@:@:@:"
    );

    WindowDelegate* delegate = [WindowDelegate alloc];
    [controller addScriptMessageHandler:delegate name:@"webview"];
    // Set delegate to window
    [window setDelegate:delegate];

    // Initialize application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Sets the app as the active app
    [NSApp activateIgnoringOtherApps:YES];

    // Add webview to window
    [window setContentView:webview];

    // Done initialization, set properties
    initDone = true;

    navigate(opts.url);
  }

  void Window::onMessage(std::function<void(String)> cb) {
    _onMessage = cb;
  }

  void Window::show () {
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
  }

  void Window::hide () {
    [window orderOut:window];
  }

  void Window::eval(const String& js) {
    [webview evaluateJavaScript:
      [NSString stringWithUTF8String:js.c_str()]
      completionHandler:nil];
  }

  void Window::navigate (const String& s) {
    [webview loadRequest:
      [NSURLRequest requestWithURL:
        [NSURL URLWithString:
          [NSString stringWithUTF8String: s.c_str()]]]];
  }

  void Window::setTitle(const String& s) {
    [window setTitle:[NSString stringWithUTF8String:s.c_str()]];
  }

  int Window::openExternal (String s) {
    NSString* nsu = [NSString stringWithUTF8String:s.c_str()];
    return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: nsu]];
  }

  String App::getCwd (String _) {
    NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
    return String([bundlePath UTF8String]);
  }

  void Window::setContextMenu (String seq, String value) {
    auto menuItems = split(value, '_');
    auto id = std::stoi(seq);

    NSPoint mouseLocation = [NSEvent mouseLocation];
    NSMenu *pMenu = [[[NSMenu alloc] initWithTitle:@"contextMenu"] autorelease];
    NSMenuItem *menuItem;
    int index = 0;

    for (auto item : menuItems) {
      auto pair = split(trim(item), ':');

      NSString* nssTitle = [NSString stringWithUTF8String:pair[0].c_str()];
      NSString* nssKey = @"";

      if (pair.size() > 1) {
        nssKey = [NSString stringWithUTF8String:pair[1].c_str()];
      }

      if (pair[0].find("---") != -1) {
        NSMenuItem *sep = [NSMenuItem separatorItem];
        [pMenu addItem:sep];
      } else {
        menuItem = [pMenu
          insertItemWithTitle:nssTitle 
          action:@selector(menuItemSelected:)
          keyEquivalent:nssKey
          atIndex:index
        ];
      }

      [menuItem setTag:id];
    
      index++;
    }

    [pMenu popUpMenuPositioningItem:pMenu.itemArray[0] atLocation:NSPointFromCGPoint(CGPointMake(mouseLocation.x, mouseLocation.y)) inView:nil];
  }

  void Window::setSystemMenu (String menu) {
    NSMenu *mainMenu;
    NSString *title;
    NSMenu *appleMenu;
    NSMenu *serviceMenu;
    NSMenu *windowMenu;
    NSMenu *editMenu;
    NSMenu *dynamicMenu;
    NSMenuItem *menuItem;

    if (NSApp == nil) {
      return;
    }

    mainMenu = [[NSMenu alloc] init];

    // Create the main menu bar
    [NSApp setMainMenu:mainMenu];
    [mainMenu release];
    mainMenu = nil;

    // Create the application menu

    // id appName = [[NSProcessInfo processInfo] processName];
    // title = [@"About " stringByAppendingString:appName];

    // deserialize the menu
    menu = replace(menu, "%%", "\n");

    // split on ;
    auto menus = split(menu, ';');

    for (auto m : menus) {
      auto menu = split(m, '\n');
      auto line = trim(menu[0]);
      if (line.empty()) continue;
      auto menuTitle = split(line, ':')[0];
      NSString* nssTitle = [NSString stringWithUTF8String:menuTitle.c_str()];
      dynamicMenu = [[NSMenu alloc] initWithTitle:nssTitle];

      for (int i = 1; i < menu.size(); i++) {
        auto line = trim(menu[i]);
        if (line.empty()) continue;
        auto parts = split(line, ':');
        auto title = parts[0];
        NSUInteger mask = 0;
        String key = "";

        if (parts.size() > 1) {
          auto accellerator = split(parts[1], '+');
          key = trim(parts[1]) == "_" ? "" : trim(accellerator[0]);

          if (accellerator.size() > 1) {
            if (accellerator[1].find("ControlOrCommand") != -1) {
              mask |= NSEventModifierFlagCommand;
            } else {
              if (accellerator[1].find("Control") != -1) mask |= NSEventModifierFlagControl;
              if (accellerator[1].find("Command") != -1) mask |= NSEventModifierFlagCommand;
            }

            if (accellerator[1].find("Option") != -1) mask |= NSEventModifierFlagOption;
          }
        }

        NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
        NSString* nssKey = [NSString stringWithUTF8String:key.c_str()];
        NSString* nssSelector = [NSString stringWithUTF8String:"menuItemSelected:"];

        if (menuTitle.compare("Edit") == 0) {
          if (title.compare("Cut") == 0) nssSelector = [NSString stringWithUTF8String:"cut:"];
          if (title.compare("Copy") == 0) nssSelector = [NSString stringWithUTF8String:"copy:"];
          if (title.compare("Paste") == 0) nssSelector = [NSString stringWithUTF8String:"paste:"];
          if (title.compare("Delete") == 0) nssSelector = [NSString stringWithUTF8String:"delete:"];
          if (title.compare("Select All") == 0) nssSelector = [NSString stringWithUTF8String:"selectAll:"];
        }

        if (title.find("About") == 0) {
          nssSelector = [NSString stringWithUTF8String:"orderFrontStandardAboutPanel:"];
        }

        if (title.find("Hide") == 0) {
          nssSelector = [NSString stringWithUTF8String:"hide:"];
        }

        if (title.find("Hide Others") == 0) {
          nssSelector = [NSString stringWithUTF8String:"hideOtherApplications:"];
        }

        if (title.find("Show All") == 0) {
          nssSelector = [NSString stringWithUTF8String:"unhideAllApplications:"];
        }

        if (title.find("Quit") == 0) {
          nssSelector = [NSString stringWithUTF8String:"terminate:"];
        }

        if (title.compare("Minimize") == 0) nssSelector = [NSString stringWithUTF8String:"performMiniaturize:"];
        if (title.compare("Zoom") == 0) nssSelector = [NSString stringWithUTF8String:"performZoom:"];

        if (title.find("---") != -1) {
          NSMenuItem *sep = [NSMenuItem separatorItem];
          [dynamicMenu addItem:sep];
        } else {
          menuItem = [dynamicMenu
            addItemWithTitle:nssTitle
            action:NSSelectorFromString(nssSelector)
            keyEquivalent:nssKey
          ];
        }

        if (mask != 0) {
          [menuItem setKeyEquivalentModifierMask: mask];
        }

        [menuItem setTag:0]; // only contextMenu uses the tag
      }

      menuItem = [[NSMenuItem alloc] initWithTitle:nssTitle action:nil keyEquivalent:@""];

      [[NSApp mainMenu] addItem:menuItem];
      [menuItem setSubmenu:dynamicMenu];
      [menuItem release];
      [dynamicMenu release];
    }
  }

  String Window::openDialog(
    bool isSave,
    bool allowDirectories,
    bool allowFiles,
    String defaultPath = "",
    String title = "")
  {
    NSURL *url;
    const char *utf8_path;
    NSSavePanel *dialog_save;
    NSOpenPanel *dialog_open;
    NSURL *default_url;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (isSave) {
      dialog_save = [NSSavePanel savePanel];
    } else {
      dialog_open = [NSOpenPanel openPanel];
    }

    if (!isSave) {
      [dialog_open setCanChooseDirectories:YES];
      [dialog_open setCanCreateDirectories:YES];
      [dialog_open setCanChooseFiles:YES];
      [dialog_open setAllowsMultipleSelection:YES];
    }

    if (defaultPath.size() > 0) {
      default_url = [NSURL fileURLWithPath:
        [NSString stringWithUTF8String: defaultPath.c_str()]];

      if (isSave) {
        [dialog_save setDirectoryURL: default_url];
        [dialog_save setNameFieldStringValue: default_url.lastPathComponent];
      } else {
        [dialog_open setDirectoryURL: default_url];
        [dialog_open setNameFieldStringValue: default_url.lastPathComponent];
      }
    }

    if (title.size() > 0) {
      NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
      if (isSave) {
        [dialog_save setTitle: nssTitle];
      } else {
        [dialog_open setTitle: nssTitle];
      }
    }

    String result = "";
    std::vector<String> paths;

    if (!isSave && [dialog_open runModal] == NSModalResponseOK) {
      NSArray* urls = [dialog_open URLs];

      for (NSURL* url in urls) {
        if ([url isFileURL]) {
          paths.push_back(String((char*) [[url path] UTF8String]));
        }
      }
    }

    [pool release];

    for (size_t i = 0, i_end = paths.size(); i < i_end; ++i) {
      result += (i ? "," : "");
      result += paths[i]; 
    }

    return String("\"" + result + "\"");
  }
}
