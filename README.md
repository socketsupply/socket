# SYNOPSIS

A fork of [webview][0] with two-way ipc and rpc that uses libuv
to spawn an arbitrary instance (ie, nodejs) as a "main" process.

# BUILD

```bash
npm run build  # build assets and compile binary
npm start      # build and start
```

# PROS

- insane fast.
- cross platform.
- about 128Kb binary.
- bitches can shut the fuck up about electron.
- loads existing front end (ie, tonic, etc).
- exact same `invokeIPC` interface.
- less dependencies and build requirements.

# CONS

- Build your own native menus.
- need to finsih building out [native file picker dialogs][1].
- need to finish ipc sequence-based protocol for aligning requests.
- default title bar (haven't really tried to figure it out).

# NOTES

[Official WebKit WebView docs][3].
[Improve scrolling on gtk][2].

[0]:https://github.com/webview/webview/blob/master/webview.h
[1]:https://github.com/javalikescript/webview-c/blob/master/webview-cocoa.c#L508
[2]:https://github.com/PerBothner/DomTerm/blob/1a8eadb111b5c4eab8dce00f5f672801af52d8f5/native/webview.cc#L33
[3]:https://developer.apple.com/documentation/webkit/wkwebview
