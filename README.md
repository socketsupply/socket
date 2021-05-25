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


# DOCS

- [Build](/build.md) How to use the build tool
- [IPC](/ipc.md) How the IPC protocol works
- [Menus](/menus.md) How to create menus
- [Updater](/updater.md) How to implement an updater
- [Window](/window.md) All global methods
- [FAQ](/faq.md) Frequently asked questions


#### INSTALL

```bash
curl -o- https://raw.githubusercontent.com/optoolco/opkit/master/bin/bootstrap.sh | bash -s install
```


# DEVELOPMENT

Compile the build program...

```sh
./bin/bootstrap.sh install
```

There is an example app written in Node.js, to run it...

```sh
./bin/build test/example
cd text/example
npm install
opkit . # run the cli tool to build and package the app
```

Then open the binary that was created by the build program...

```sh
open ./dist/Operator.app # macOS
```

```sh
./dist/operator_v0.0.1-1_x64/opt/Operator/operator # linux
```

```sh
dist\operator.exe # win32
```

[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm

[5]:https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
