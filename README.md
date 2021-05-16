# SYNOPSIS

Build fast, light-weight UIs with any backend.


# DESCRIPTION

A webview that uses libuv to spawn and communicate with
any arbitrary child process (ie, node, rust, etc).


# HIGHLIGHTS

- Very fast startup and build times
- Tiny memory, cpu and binary
- Runs on macOS, linux, windows
- System Menus, Context Menus, File Dialogs
- Webkit web inspector
- The only build requiremnt a modern C++ compiler


# GET STARTED

"Modern Compiler" means whatever ships with latest MacOS or, on linux
it means something like `clang-12 libc++1-12-dev libc++abi-12-dev libwebkit2gtk-4.0-dev`.

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
const result = await send('honk')
assert(result === 'goose')
```

#### In the backend process (node.js example)

```js
import { send, receive } from './ipc.js'

receive(async data => {
  if (data === 'honk') send('goose')
})
```

# DEVELOPMENT

Compile the build program...

```sh
`echo $CXX` src/build.cc -o bin/build -std=c++2a -stdlib=libc++"
```

Build the example...

```sh
./bin/build test/example
```

Then open the binary...

```sh
open test/example/dist/Operator.app # macOS
```

```sh
./test/example/dist/operator_v0.0.1-1_x64/opt/Operator/operator # linux
```

Then run the build

[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm

[5]:https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
