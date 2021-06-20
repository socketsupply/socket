#include <AppKit/AppKit.h>
#include <WebKit/WebKit.h>
#include <WebKit/WebView.h>
#include <WebKit/WKWebView.h>
#include "util.h"

typedef const struct OpaqueWKPage* WKPageRef;
typedef const struct OpaqueWKPageGroup* WKPageGroupRef;
typedef const struct OpaqueWKInspector* WKInspectorRef;
typedef const struct OpaqueWKPreferences* WKPreferencesRef;

// https://github.com/WebKit/WebKit/blob/main/Source/WebKit/UIProcess/API/C/WKInspector.h
WKInspectorRef WKPageGetInspector(WKPageRef pageRef);
void WKInspectorShow(WKInspectorRef inspector);
// void WKInspectorShowConsole(WKInspectorRef inspector);

void showInspector(void* m_webview) {
  WKWebView* webView = (WKWebView*) m_webview;

  // WKInspectorShow(WKPageGetInspector(webView.?????));

  // auto inspector = [WKInspector alloc];
  // [inspector initWithInspectedWebView:webView];

  // [inspector detach:webView];
  // [inspector show:webView];
}

std::string getCwd (std::string _) {
  NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
  return std::string([bundlePath UTF8String]);
}

std::vector<std::string> getMenuItemDetails (void* item) {
  id menuItem = (id) item;
  std::string title = [[menuItem title] UTF8String];
  std::string state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
  std::string parent = [[[menuItem menu] title] UTF8String];
  std::string seq = std::to_string([menuItem tag]);
  std::vector<std::string> vec = { title, state, parent, seq };
  return vec;
}

void showWindow () {
  [NSApp.windows[0] makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
}

void hideWindow () {
  [[NSApplication sharedApplication] hide:nil];
}

int openExternalURL (std::string url) {
  NSString* nsu = [NSString stringWithUTF8String:url.c_str()];
  return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: nsu]];
}

//
// WTF how do you open the web inspector from code
//
//void showInspector (void* m_webview) {
  //WebInspector *inspector = [WebInspector alloc];

  // WKWebView* webView = (WKWebView*) m_webview;

  // WKInspectorShow(WKPageGetInspector((WKPageRef) webView.subviews.firstObject));

  //id m_inspector = [[WebInspector alloc] initWithWebView: webView];
  //[m_inspector detach:webView];

  // if(console){
  //  [m_inspector showConsole:webView];
  // }
  // else {
  //  [m_inspector show:m_webView]; */
//}

bool createNativeContextMenu (std::string seq, std::string value) {
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
  return true;
}

void setWindowColor (void* w, float r, float g, float b, float a) {
  NSWindow* win = (id) w;

  [win
    setBackgroundColor: [
      NSColor
        colorWithDeviceRed: (CGFloat) r
        green: (CGFloat) g
        blue: (CGFloat) b
        alpha: (CGFloat) 1.0
    ]
  ];
}


void createNativeMenu (std::string menu) {
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
      std::string key = "";

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

std::string createNativeDialog(
  int flags,
  const char *filters,
  const char *default_path,
  const char *default_name)
{
  NSURL *url;
  const char *utf8_path;
  NSSavePanel *dialog;
  NSOpenPanel *open_dialog;
  NSURL *default_url;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  if (flags & NOC_FILE_DIALOG_OPEN) {
    dialog = open_dialog = [NSOpenPanel openPanel];
  } else {
    dialog = [NSSavePanel savePanel];
  }

  if (flags & NOC_FILE_DIALOG_DIR) {
    [open_dialog setCanChooseDirectories:YES];
    [open_dialog setCanCreateDirectories:YES];
    [open_dialog setCanChooseFiles:YES];
    [open_dialog setAllowsMultipleSelection:YES];
  }

  if (default_path) {
    default_url = [NSURL fileURLWithPath: [NSString stringWithUTF8String:default_path]];
    [dialog setDirectoryURL:default_url];
    [dialog setNameFieldStringValue:default_url.lastPathComponent];
  }

  std::string result;
  std::vector<std::string> paths;

  if ([dialog runModal] == NSModalResponseOK) {
    NSArray* urls = [dialog URLs];

    for (NSURL* url in urls) {
      if ([url isFileURL]) {
        paths.push_back(std::string((char*) [[url path] UTF8String]));
      }
    }
  }

  [pool release];

  for (size_t i = 0, i_end = paths.size(); i < i_end; ++i) {
    result += (i ? "," : "");
    result += paths[i]; 
  }

  return result;
}
