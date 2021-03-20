# SYNOPSIS

A fork of [webview][00] with two-way ipc and rpc that uses libuv
to spawn an arbitrary instance (ie, nodejs) as a "main" process.

```
✓ Fast, light-weight 2D UIs with an IPC to any backend.
✕ Not intended for games (use a proper framework).
```

# BUILD

```bash
./bin/build <target-directory>
```

# PROS

- Very fast startup and build times
- < 0.1% memory baseline
- < 0% cpu baseline
- About 150Kb baseline binary
- Works the same on macOS, linux, windows
- Basically it's Webkit
- Less dependencies and build requirements
- Use the regular web developer tools

# CONS

- Less full featured than electron

# NOTES

[Improve scrolling on gtk][2].

[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm

https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
