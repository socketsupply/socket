#ifdef __APPLE__
#import "platform.h"

#include <AppKit/AppKit.h>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

// #define SharedContextMenuTarget  [ContextMenuTarget sharedInstance]

std::string getCwd () {
  NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
  auto str = std::string([bundlePath UTF8String]);
  return str;
}

std::vector<std::string> getMenuItemDetails (void* item) {
  id menuItem = (id) item;
  std::string id = std::to_string([menuItem tag]);
  std::string title = [[menuItem title] UTF8String];
  std::string state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
  std::vector<std::string> vec = { id, title, state };
  return vec;
}

void createMenu (std::string menu) {
  NSString *title;
  NSMenu *appleMenu;
  NSMenu *serviceMenu;
  NSMenu *windowMenu;
  NSMenu *editMenu;
  NSMenu *dynamicMenu;
  NSMenuItem *menuItem;
  NSMenu *mainMenu;

  if (NSApp == nil) {
    return;
  }

  mainMenu = [[NSMenu alloc] init];

  // Create the main menu bar
  [NSApp setMainMenu:mainMenu];

  [mainMenu release];  // we're done with it, let NSApp own it.
  mainMenu = nil;

  // Create the application menu

  id appName = [[NSProcessInfo processInfo] processName];
  appleMenu = [[NSMenu alloc] initWithTitle:@""];

  // Add menu items
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

  [appleMenu addItem:[NSMenuItem separatorItem]];

  title = [@"Quit " stringByAppendingString:appName];
  [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

  // Put menu into the menubar
  menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  [menuItem setSubmenu:appleMenu];
  [[NSApp mainMenu] addItem:menuItem];
  [menuItem release];

  // Tell the application object that this is now the application menu
  // [NSApp setAppleMenu:appleMenu];
  [appleMenu release];

  // Create the window menu
  editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];

  //Add menu items
  [editMenu addItemWithTitle: @"Cut" action: @selector(cut:) keyEquivalent: @"x"];
  [editMenu addItemWithTitle: @"Copy" action: @selector(copy:) keyEquivalent: @"c"];
  [editMenu addItemWithTitle: @"Paste" action: @selector(paste:) keyEquivalent: @"v"];
  [editMenu addItemWithTitle: @"Delete" action: @selector(delete:) keyEquivalent: @""];
  [editMenu addItemWithTitle: @"Select All" action: @selector(selectAll:) keyEquivalent: @"a"];

  // Put menu into the menubar
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
  [[NSApp mainMenu] addItem:menuItem];

  [menuItem setSubmenu:editMenu];
  [menuItem release];
  [editMenu release];

  // deserialize the menu
  std::replace(menu.begin(), menu.end(), '_', '\n');

  // split on ;
  auto menus = split(menu, ';');

  for (auto m : menus) {
    auto menu = split(m, '\n');
    auto title = split(trim(menu[0]), ':')[0];
    NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
    dynamicMenu = [[NSMenu alloc] initWithTitle:nssTitle];

    for (int i = 1; i < menu.size(); i++) {
      auto parts = split(trim(menu[i]), ':');
      auto title = parts[0];
      auto pair = split(trim(parts[1]), ' ');
      auto id = std::stoi(pair[0]);
      auto key = pair.size() == 2 ? pair[1] : "";

      NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
      NSString* nssKey = [NSString stringWithUTF8String:key.c_str()];

      menuItem = [dynamicMenu
        addItemWithTitle:nssTitle
        action:@selector(menuItemSelected:)
        keyEquivalent:nssKey
      ];

      [menuItem setTag:id];
    }

    menuItem = [[NSMenuItem alloc] initWithTitle:nssTitle action:nil keyEquivalent:@""];

    [[NSApp mainMenu] addItem:menuItem];
    [menuItem setSubmenu:dynamicMenu];
    [menuItem release];
    [dynamicMenu release];
  }

  // Create the window menu
  windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

  // Add menu items
  // [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
  // [windowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];

  // Put menu into the menubar
  menuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];

  // [menuItem setSubmenu:windowMenu];
  // [[NSApp mainMenu] addItem:menuItem];
  [menuItem release];

  // Tell the application object that this is now the window menu
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
