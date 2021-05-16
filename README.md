# SYNOPSIS

Build and package fast, light-weight web-based UIs with any backend.


# HIGHLIGHTS

- Very fast startup and build times
- Tiny memory, cpu and binary
- Runs on macOS, linux, windows
- System Menus, Context Menus, File Dialogs
- Webkit web inspector
- The only build requiremnt a modern C++ compiler
- Smaller than and with fewer dependencies than tauri or electron
- Runs any arbitrary backend process


# GET STARTED

"Modern Compiler" means whatever ships with latest MacOS or, on linux
it means something like `clang-12 libc++1-12-dev libc++abi-12-dev libwebkit2gtk-4.0-dev`.


#### INSTALL

```bash
curl -o- https://raw.githubusercontent.com/optoolco/opkit/master/install.sh | bash
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

[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm

[5]:https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
