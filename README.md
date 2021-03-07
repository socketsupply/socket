# SYNOPSIS

A fork of [`webview`][0] with two-way ipc and rpc that uses libuv
to spawn an arbitrary instance (ie, nodejs) as a "main" process.

# BUILD

```bash
%npm run build && (cd build && ./operator)
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

- NIH
- need to finsih building out native file picker dialogs.
- need to finish ipc sequence-based protocol for aligning requests.
- default title bar (haven't really tried to figure it out).

[0]:https://github.com/webview/webview/blob/master/webview.h
