# SYNOPSIS

Build fast, light-weight 2D UIs with any backend.


# DESCRIPTION

A webview that uses libuv to spawn and communicate with
any arbitrary child process (ie, node, rust, etc).


# HIGHLIGHTS

- Very fast startup and build times
- < 0.1% memory baseline
- < 0% cpu baseline
- About 150Kb baseline binary
- Works the same on macOS, linux, windows
- System Menus, Context Menus, File Dialogs
- Use the regular web development tools
- The only build requiremnt a modern C++ compiler


# GET STARTED

#### INSTALL

```bash
curl -o- https://raw.githubusercontent.com/optoolco/opkit/master/install.sh | bash
```


# DOCS

## Menus

#### Context Menus

Dynamically build a context menu and await the user's selection.

```js
const value = await contextMenu({
  'Download': 'd',
  'Wizard': 'w',
  'Share': 's'
})

assert(value === { title: 'Wizard', parent: 'contextMenu', state: 0 })
```

#### System Menus

System menus are created at build time. Your project needs a `menu.config`
in the root folder. The semi colon is significant indicates the end of the menu.

```js
Edit:
  Cut: x
  Copy: c
  Paste: v
  Delete: _
  Select All: a;

Other:
  Apple: _
  Another Test: t
```

When a selection is made the `menuItemSelected` event will be fired in the
browser. It will contain the same info as the result from the context menu.

## IPC

#### From the browser

```js
const result = await invokeIPC('honk')
assert(result === 'goose')
```

#### In the backend process (node.js example)

```js
import ipc from './ipc.js'

ipc.receive(async data => {
  if (data === 'honk') ipc.send('goose')
})
```


[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm

[5]:https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
