# SYNOPSIS

Build and package fast, light-weight web-based UIs with any backend.


# HIGHLIGHTS

- Instant startup, first UI paint in <0.3s
- <1s build time baseline
- Tiny memory, cpu and binary baselines
- Runs on macOS, linux, windows
- System Menus, Context Menus, File Dialogs
- Webkit web inspector
- The only build requiremnt a modern C++ compiler toolchain
- Zero dependencies (except platform frameworks, gtk, cocoa, etc)
- Smaller than and with fewer dependencies than tauri or electron
- Runs any arbitrary backend process

# GET STARTED

"Modern Compiler" means whatever ships with latest MacOS or, on linux
it means something like `clang-12 libc++1-12-dev libc++abi-12-dev libwebkit2gtk-4.0-dev`.

See more docs [here](https://github.com/optoolco/opkit/blob/master/docs/index.md).

#### INSTALL (TODO)

```bash
curl -o- https://raw.githubusercontent.com/optoolco/opkit/master/bootstrap.sh install | bash
```


# DEVELOPMENT

Compile the build program...

```sh
./bootstrap.sh
```

Build the example...

```sh
./bin/build test/example
cd text/example
npm install
```

Then open the binary...

```sh
open test/example/dist/Operator.app # macOS
```

```sh
./test/example/dist/operator_v0.0.1-1_x64/opt/Operator/operator # linux
```

# FAQ

#### Why the F is it in C++ instead of Rust?

Rust is incredible. Write your backend in Rust. Even write your front end in Rust
and load it as WASM! This project is in C++ because webkit is, Cocoa is, GTK is,
and Windows is. Writing this in C++ means less context switching, fewer intermediate
steps.

#### What about keyboard accelerators?

All menues raise events in the front-end. So should keyboard accelerators.
Your accelerators, and menu items can all use `addEventListener` and then
send a message to the backend if needed.

#### I need feature-X, but Opkit doesn't support that, can you make it?

You can make a PR. But the goal is not to solve the all problems for all
use cases. The goal is to be minimal. Just the essentials. Stay fast and
stay simple. Electron or Tauri might have what you are looking for.


[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm

[5]:https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
