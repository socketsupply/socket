//
// ====================================================================
//
// This implementation uses Cocoa WKWebView backend on macOS. It is
// written using ObjC runtime and uses WKWebView class as a browser runtime.
// You should pass "-framework Webkit" flag to the compiler.
//
// ====================================================================
//
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc-runtime.h>

#define NSBackingStoreBuffered 2
#define NSWindowStyl0MaskHUDWindow 1 << 13
#define NSWindowStyleMaskResizable 8
#define NSWindowStyleMaskMiniaturizable 4
#define NSWindowStyleMaskTitled 1
#define NSTexturedBackgroundWindowMask 1 << 8
#define NSWindowStyleMaskTexturedBackground 1 << 8
#define NSWindowStyleMaskUnifiedTitleAndToolbar 1 << 12
#define NSWindowTitleHidden 1
#define NSFullSizeContentViewWindowMask 1 << 15
#define NSWindowStyleMaskClosable 2
#define NSApplicationActivationPolicyRegular 0
#define WKUserScriptInjectionTimeAtDocumentStart 0

void createMenu (std::string);

namespace webview {
  SEL NSSelector(const char *s) {
    return sel_registerName(s);
  }

  id NSClass(const char *s) {
    return (id) objc_getClass(s);
  }

  id NSString (const char *s) {
    return ((id(*) (id, SEL, const char *)) objc_msgSend)(
      NSClass("NSString"),
      NSSelector("stringWithUTF8String:"),
      s
    );
  }

  class cocoa_wkwebview_engine {
    public:

    cocoa_wkwebview_engine(bool debug, void *window) {
      // Application
      id app = ((id(*)(id, SEL))objc_msgSend)(
        NSClass("NSApplication"),
        NSSelector("sharedApplication")
      );

      ((void (*)(id, SEL, long)) objc_msgSend)(
        app,
        NSSelector("setActivationPolicy:"),
        NSApplicationActivationPolicyRegular
      );

      // Delegate
      auto cls = objc_allocateClassPair((Class) NSClass("NSResponder"), "AppDelegate", 0);

      class_addProtocol(cls, objc_getProtocol("NSTouchBarProvider"));

      class_addMethod(
        cls,
        NSSelector("applicationShouldTerminateAfterLastWindowClosed:"),
        (IMP)(+[](id, SEL, id) -> BOOL { return 1; }),
        "c@:@"
      );

      class_addMethod(
        cls,
        NSSelector("menuItemSelected:"),
        (IMP)(+[](id self, SEL _cmd, id item) {
          auto w = (cocoa_wkwebview_engine*) objc_getAssociatedObject(self, "webview");
          assert(w);

          auto vec = getMenuItemDetails(item);
          auto title = vec[0];
          auto state = vec[1];
          auto parent = vec[2];
          auto seq = vec[3];

          w->eval("(() => {"
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

      class_addMethod(
        cls,
        NSSelector("themeChangedOnMainThread"),
        (IMP)(+[](id self) {
          auto w = (cocoa_wkwebview_engine*) objc_getAssociatedObject(self, "webview");
          assert(w);

          w->eval("(() => {"
            "  const event = new window.CustomEvent('themeChanged');"
            "  window.dispatchEvent(event);"
            "})()"
          );
        }),
        "v"
      );

      class_addMethod(
        cls,
        NSSelector("userContentController:didReceiveScriptMessage:"),
        (IMP)(+[](id self, SEL, id, id msg) {
          auto w = (cocoa_wkwebview_engine*) objc_getAssociatedObject(self, "webview");
          assert(w);

          w->on_message(((const char* (*)(id, SEL))objc_msgSend)(
            ((id (*)(id, SEL)) objc_msgSend)(
              msg,
              NSSelector("body")
            ),
            NSSelector("UTF8String")
          ));
        }),
        "v@:@@"
      );

      objc_registerClassPair(cls);

      auto delegate = ((id(*)(id, SEL))objc_msgSend)(
        (id)cls,
        NSSelector("new")
      );

      objc_setAssociatedObject(
        delegate,
        "webview",
        (id)this,
        OBJC_ASSOCIATION_ASSIGN
      );

      ((void (*)(id, SEL, id))objc_msgSend)(
        app,
        sel_registerName("setDelegate:"),
        delegate
      );

      addListenerThemeChange(delegate);

      // Main window
      if (window == nullptr) {
        m_window = ((id(*)(id, SEL))objc_msgSend)(
          NSClass("NSWindow"),
          NSSelector("alloc")
        );
        
        m_window = ((id(*)(id, SEL, CGRect, int, unsigned long, int))objc_msgSend)(
          m_window,
          NSSelector("initWithContentRect:styleMask:backing:defer:"),
          CGRectMake(0, 0, 0, 0),
          0,
          NSBackingStoreBuffered,
          0
        );
      } else {
        m_window = (id)window;
      }

      // Webview
      auto config = ((id (*)(id, SEL)) objc_msgSend)(
        NSClass("WKWebViewConfiguration"),
        NSSelector("new")
      );

      m_manager = ((id (*)(id, SEL)) objc_msgSend)(
        config,
        NSSelector("userContentController")
      );

      m_webview = ((id (*)(id, SEL)) objc_msgSend)(
        NSClass("WKWebView"),
        NSSelector("alloc")
      );

      if (debug) {
        //
        // [[config preferences] setValue:@YES forKey:@"developerExtrasEnabled"];
        // 
        ((id(*)(id, SEL, id, id))objc_msgSend)(
          ((id(*)(id, SEL))objc_msgSend)(
            config,
            NSSelector("preferences")
          ),
          NSSelector("setValue:forKey:"),
          ((id(*)(id, SEL, BOOL))objc_msgSend)(
            NSClass("NSNumber"),
            NSSelector("numberWithBool:"),
            1),
          NSString("developerExtrasEnabled")
        );
      }

      //
      // [[config preferences] setValue:@YES forKey:@"fullScreenEnabled"];
      //
      ((id(*)(id, SEL, id, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)(
          config,
          NSSelector("preferences")
        ),
        NSSelector("setValue:forKey:"),
        ((id(*)(id, SEL, BOOL))objc_msgSend)(
          NSClass("NSNumber"),
          NSSelector("numberWithBool:"),
          1
        ),
        NSString("fullScreenEnabled")
      );

      //
      // [[config preferences] setValue:@YES forKey:@"allowFileAccessFromFileURLs"];
      //
      ((id(*)(id, SEL, id, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)(
          config,
          NSSelector("preferences")
        ),
        NSSelector("setValue:forKey:"),
        ((id(*)(id, SEL, BOOL))objc_msgSend)(
          NSClass("NSNumber"),
          NSSelector("numberWithBool:"),
          1
        ),
        NSString("allowFileAccessFromFileURLs")
      );

      //
      // [[config preferences] setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
      //
      ((id(*)(id, SEL, id, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)(
          config,
          NSSelector("preferences")
        ),
        NSSelector("setValue:forKey:"),
        ((id(*)(id, SEL, BOOL))objc_msgSend)(
          NSClass("NSNumber"),
          NSSelector("numberWithBool:"),
          1
        ),
        NSString("javaScriptCanAccessClipboard")
      );

      //
      // [[config preferences] setValue:@YES forKey:@"DOMPasteAllowed"];
      //
      ((id(*)(id, SEL, id, id))objc_msgSend)(
        ((id(*)(id, SEL))objc_msgSend)(
          config,
          NSSelector("preferences")
        ),
        NSSelector("setValue:forKey:"),
        ((id(*)(id, SEL, BOOL))objc_msgSend)(
          NSClass("NSNumber"),
          NSSelector("numberWithBool:"),
          1
        ),
        NSString("DOMPasteAllowed")
      );

      ((void (*)(id, SEL, CGRect, id))objc_msgSend)(
        m_webview,
        NSSelector("initWithFrame:configuration:"),
        CGRectMake(0, 0, 0, 0),
        config
      );

      // initTitleBar();

      ((void (*)(id, SEL, id, id))objc_msgSend)(
        m_manager,
        NSSelector("addScriptMessageHandler:name:"),
        delegate,
        NSString("external")
      );

      init(R"script(
        window.external = {
          invoke: s => {
            window.webkit.messageHandlers.external.postMessage(s)
          }
        }
       )script");

      ((void (*)(id, SEL, id))objc_msgSend)(
        m_window,
        NSSelector("setContentView:"),
        m_webview
      );

      ((void (*)(id, SEL, id))objc_msgSend)(
        m_window,
        NSSelector("makeKeyAndOrderFront:"),
        nullptr
      );
    }

    ~cocoa_wkwebview_engine() { close(); }
    void *window() { return (void *)m_window; }

    void terminate() {
      close();

      ((void (*)(id, SEL, id))objc_msgSend)(
        NSClass("NSApp"),
        NSSelector("terminate:"),
        nullptr
      );
    }

    void run() {
      id app = ((id(*)(id, SEL))objc_msgSend)(
        NSClass("NSApplication"),
        NSSelector("sharedApplication")
      );

      dispatch([&]() {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(
          app,
          NSSelector("activateIgnoringOtherApps:"),
          1
        );
      });

      ((void (*)(id, SEL))objc_msgSend)(app, NSSelector("run"));
    }

    void dispatch(std::function<void()> f) {
      dispatch_async_f(dispatch_get_main_queue(), new dispatch_fn_t(f),
       (dispatch_function_t)([](void *arg) {
         auto f = static_cast<dispatch_fn_t *>(arg);
         (*f)();
         delete f;
       })
      );
    }

    void set_title(const std::string title) {
      ((void (*)(id, SEL, id))objc_msgSend)(
        m_window,
        NSSelector("setTitle:"),
        ((id(*)(id, SEL, const char *))objc_msgSend)(
          NSClass("NSString"),
          NSSelector("stringWithUTF8String:"),
          title.c_str()
        )
      );

      setTitle(m_window);
    }

    void set_size(int width, int height, int hints) {
      auto style = 
        NSWindowStyleMaskClosable |
        NSWindowStyleMaskTitled |
        // NSFullSizeContentViewWindowMask |
        // NSTexturedBackgroundWindowMask |
        // NSWindowStyleMaskTexturedBackground |
        // NSWindowTitleHidden |
        NSWindowStyleMaskMiniaturizable;

      if (hints != WEBVIEW_HINT_FIXED) {
        style = style | NSWindowStyleMaskResizable;
      }

      ((void (*)(id, SEL, unsigned long)) objc_msgSend)(
        m_window,
        NSSelector("setStyleMask:"),
        style
      );

      if (hints == WEBVIEW_HINT_MIN) {
        ((void (*)(id, SEL, CGSize)) objc_msgSend)(
          m_window,
          NSSelector("setContentMinSize:"),
          CGSizeMake(width, height)
        );
      } else if (hints == WEBVIEW_HINT_MAX) {
        ((void (*)(id, SEL, CGSize)) objc_msgSend)(
          m_window,
          NSSelector("setContentMaxSize:"),
          CGSizeMake(width, height)
        );
      } else {
        ((void (*)(id, SEL, CGRect, BOOL, BOOL)) objc_msgSend)(
          m_window,
          NSSelector("setFrame:display:animate:"),
          CGRectMake(0, 0, width, height),
          1,
          0
        );
      }

      ((void (*)(id, SEL))objc_msgSend)(m_window, NSSelector("center"));
      ((void (*)(id, SEL))objc_msgSend)(m_window, NSSelector("setHasShadow:"));

      // setWindowColor(m_window, 0.1, 0.1, 0.1, 0.0);

      ((void (*)(id, SEL, BOOL))objc_msgSend)(m_window, NSSelector("setTitlebarAppearsTransparent:"), 1);
      ((void (*)(id, SEL, BOOL))objc_msgSend)(m_window, NSSelector("setOpaque:"), 1);

      // ((void (*)(id, SEL, BOOL))objc_msgSend)(m_window, "setTitleVisibility:"_sel, 1);
      // ((void (*)(id, SEL, BOOL))objc_msgSend)(m_window, "setMovableByWindowBackground:"_sel, 1);
      // setWindowButtonsOffset:NSMakePoint(12, 10)
      // setTitleVisibility:NSWindowTitleHidden
    }

    void menu(const std::string menu) {
      createMenu(menu);
    }

    void navigate(const std::string url) {
      auto nsurl = ((id(*)(id, SEL, id))objc_msgSend)(
        NSClass("NSURL"),
        NSSelector("URLWithString:"),
        ((id(*)(id, SEL, const char *))objc_msgSend)(
          NSClass("NSString"),
          NSSelector("stringWithUTF8String:"),
          url.c_str()
        )
      );

      ((void (*)(id, SEL, id))objc_msgSend)(
        m_webview,
        NSSelector("loadRequest:"),
        ((id(*)(id, SEL, id))objc_msgSend)(
          NSClass("NSURLRequest"),
          NSSelector("requestWithURL:"),
          nsurl
        )
      );
    }

    void init(const std::string js) {
      // Equivalent Obj-C:
      // [m_manager addUserScript:[[WKUserScript alloc] initWithSource:[NSString stringWithUTF8String:js.c_str()] injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES]]
      ((void (*)(id, SEL, id)) objc_msgSend)(
        m_manager,
        NSSelector("addUserScript:"),
        ((id (*)(id, SEL, id, long, BOOL)) objc_msgSend)(
          ((id (*)(id, SEL)) objc_msgSend)(
            NSClass("WKUserScript"),
            NSSelector("alloc")
          ),
          NSSelector("initWithSource:injectionTime:forMainFrameOnly:"),
          ((id (*)(id, SEL, const char *)) objc_msgSend)(
            NSClass("NSString"),
            NSSelector("stringWithUTF8String:"),
            js.c_str()
          ),
          WKUserScriptInjectionTimeAtDocumentStart,
          1
        )
      );
    }

    void eval(const std::string js) {
      ((void (*)(id, SEL, id, id)) objc_msgSend)(
        m_webview,
        NSSelector("evaluateJavaScript:completionHandler:"),
        ((id(*)(id, SEL, const char *)) objc_msgSend)(
          NSClass("NSString"),
          NSSelector("stringWithUTF8String:"),
          js.c_str()
        ),
        nullptr
      );
    }

  private:
    virtual void on_message(const std::string msg) = 0;
    void close() { ((void (*)(id, SEL))objc_msgSend)(m_window, NSSelector("close")); }
    id m_window;
    id m_webview;
    id m_manager;
  };

  using browser_engine = cocoa_wkwebview_engine;
} // namespace webview
