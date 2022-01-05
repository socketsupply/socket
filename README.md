# SYNOPSIS

A minimalist framework for building and packaging light and fast,
UIs that can run any arbitrary backend process.


# HIGHLIGHTS

- Instant startup, first UI paint in <0.3s
- <1s build time baseline
- Tiny memory, cpu and binary baselines
- Runs on MacOS, Linux, Windows, iOS, and (almost Android)
- System Menus, Context Menus, File Dialogs
- Proper sandboxing features
- Webkit web inspector
- The only build requiremnt a modern C++ compiler toolchain
- Zero dependencies (except platform frameworks, gtk, cocoa, etc)
- Smaller than and with fewer dependencies than `Tauri` or `Electron`
- Runs any arbitrary backend process


# DOCS

- [API](/docs/api.md) All methods
- [Build](/docs/build.md) How to use the build tool
- [IPC](/docs/ipc.md) How the IPC protocol works
- [Menus](/docs/menus.md) How to create menus
- [Trouble Shooting Guide](/docs/troubleshooting.md) Help with compiling
- [Updater](/docs/updater.md) How to implement an updater
- [FAQ](/docs/faq.md) Frequently asked questions


# INSTALL
If you want to use Opkit to build apps, you can use this install script.
It's a plain text script, please inspect it before running it.

## Linux or MacOS Bash/Zsh
```bash
curl -o- https://raw.githubusercontent.com/operatortc/opkit/master/bin/bootstrap.sh | bash -s install
```

## Windows Powershell
```ps1
& $([scriptblock]::Create((New-Object Net.WebClient).DownloadString("https://raw.githubusercontent.com/operatortc/opkit/master/bin/bootstrap.ps1"))) -Install
```


# DEVELOPMENT ON OPKIT

```sh
git clone git@github.com:operatortc/opkit.git
cd opkit
```

If you want to hack on the Opkit project itself, clone this repo and install
the CLI tool (unless you already did this with curl).

### MacOS, iOS, Android, and Linux

```sh
./bin/bootstrap.sh install local
```

### Windows

```sh
powershell ./bin/bootstrap.ps1 install local
```

There is an example app written in Node.js, to build and run it...
Using the `-r` flag will find the built artifact and run it.

```sh
cd test/example
npm install
opkit . -r
```

[02]:https://developer.apple.com/forums/thread/128166
[01]:https://developer.apple.com/documentation/webkit/wkwebview
[00]:https://developer.apple.com/videos/play/wwdc2020/10188/
[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[4]:https://github.com/electron/electron/blob/6b6ffbdd107f4633b2b70d0e41be64aa65efc540/shell/browser/ui/cocoa/electron_menu_controller.mm
[5]:https://github.com/progrium/macdriver/blob/5eac15a75a75a7f275eca60ba2e64e6f29f16061/cocoa/NSWindow.go
