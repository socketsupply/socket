# IPC

The IPC protocol is based on a simple URI-like scheme.

```uri
ipc://command?key1=value1&key2=value2...
```

Values are encoded using `encodeURIComponent`.

Here is an [implementation reference][0].

[0]: https://github.com/socketsupply/opkit/blob/master/test/example/src/main/ipc.js
