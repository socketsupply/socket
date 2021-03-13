#ifdef __APPLE__
#import "platform.h"

#include <AppKit/AppKit.h>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <ostream>

// #define SharedContextMenuTarget  [ContextMenuTarget sharedInstance]

class Action {
  public:
    void init();
    void trigger();
    bool isEnabled();
};

void Action::init () {
}

void Action::trigger () {
}

bool Action::isEnabled () {
  return true;
}

void doThing () {
  printf("X");
}

/* @interface Target : NSObject {
  Action* action_;
}
- (id) initWithAction: (Action*) action;
- (void) clicked;
@end

@implementation Target
- (id) init {
  return [super init];
}

- (id) initWithAction: (Action*)action {
  action_ = action;
  return self;
}

- (BOOL) validateMenuItem: (NSMenuItem*) menuItem {
  // This is called when the menu is shown.
  return action_->isEnabled();
}

- (void) clicked {
  action_->trigger();
}

@end */

/* @interface Target : NSObject {
@end

@implementation Target
- (void)play {
}
@end */

std::string getCwd () {
  NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
  auto str = std::string([bundlePath UTF8String]);
  return str;
}

void createMenu () {
  NSString *title;
  NSMenu *appleMenu;
  NSMenu *serviceMenu;
  NSMenu *windowMenu;
  NSMenu *editMenu;
  NSMenuItem *menuItem;
  NSMenu *mainMenu;

  if (NSApp == nil) {
      return;
  }

  mainMenu = [[NSMenu alloc] init];

  /* Create the main menu bar */
  [NSApp setMainMenu:mainMenu];

  [mainMenu release];  /* we're done with it, let NSApp own it. */
  mainMenu = nil;

  /* Create the application menu */

  id appName = [[NSProcessInfo processInfo] processName];
  appleMenu = [[NSMenu alloc] initWithTitle:@""];

  /* Add menu items */
  title = [@"About " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  [appleMenu addItemWithTitle:@"Preferences…" action:nil keyEquivalent:@","];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  serviceMenu = [[NSMenu alloc] initWithTitle:@""];
  menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""];
  [menuItem setSubmenu:serviceMenu];

  [NSApp setServicesMenu:serviceMenu];
  [serviceMenu release];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  title = [@"Hide " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

  menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
  [menuItem setKeyEquivalentModifierMask:(NSEventModifierFlagOption|NSEventModifierFlagCommand)];

  [appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

  [appleMenu
    addItemWithTitle:@"Test"
    action:@selector(doThing)
    keyEquivalent:@""
  ];

  [appleMenu addItem:[NSMenuItem separatorItem]];

  title = [@"Quit " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

  /* Put menu into the menubar */
  menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  [menuItem setSubmenu:appleMenu];
  [[NSApp mainMenu] addItem:menuItem];
  [menuItem release];

  /* Tell the application object that this is now the application menu */
  // [NSApp setAppleMenu:appleMenu];
  [appleMenu release];

  /* Create the window menu */
  editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];

  /* Add menu items */
  [editMenu addItemWithTitle: @"Cut" action: @selector(cut:) keyEquivalent: @"x"];
  [editMenu addItemWithTitle: @"Copy" action: @selector(copy:) keyEquivalent: @"c"];
  [editMenu addItemWithTitle: @"Paste" action: @selector(paste:) keyEquivalent: @"v"];
  [editMenu addItemWithTitle: @"Delete" action: @selector(delete:) keyEquivalent: @""];
  [editMenu addItemWithTitle: @"Select All" action: @selector(selectAll:) keyEquivalent: @"a"];

  /* Put menu into the menubar */
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
  [menuItem setSubmenu:editMenu];
  [[NSApp mainMenu] addItem:menuItem];
  [menuItem release];

  [menuItem setSubmenu:editMenu];
  [editMenu release];

  /* Create the window menu */
  windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

  /* Add menu items */
  [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
  [windowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];

  /* Put menu into the menubar */
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
  [menuItem setSubmenu:windowMenu];
  [[NSApp mainMenu] addItem:menuItem];
  [menuItem release];

  /* Tell the application object that this is now the window menu */
  [NSApp setWindowsMenu:windowMenu];
  [windowMenu release];
}

std::string dialog_open(
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

#endif
