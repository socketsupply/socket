# Socket Runtime

### Description

Socket is a client-side runtime for creating native cross-platform software on
mobile and desktop using HTML, CSS, and JavaScript. It also exposes primitives
needed for building peer-to-peer and local-first applications, such as Bluetooth,
UDP and File I/O.

### Documentation

<!-- [![Socket SDK CI](https://github.com/socketsupply/socket/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/socketsupply/socket/actions/workflows/ci.yml) -->

Please read (and contribute to) the documentation [here](https://sockets.sh).

### Testing

See the [`io`][0] library for the unit and integration test suite.

### Development

If you want to contribute to the Socket Runtime project itself, please connect
with any current project contributor.

### Roadmap

```mermaid
gantt
    title Project Roadmap
    dateFormat  YYYY-MM-DD
    axisFormat  %Y-%m
    todayMarker off

    section All
    Cross Compiling: X0, 2023-03, 20d

    section Android
    Optize Build Steps: A1, 14d

    section iOS
    Security Crash Messaging: I0, 2023-01, 2d

    section MacOS
    Security Crash Messaging: M0, 2023-01, 2d

    section Linux
    Finish Bluetooth: L2, after A1, 14d

    section Windows
    Core Integration: W0, 15d
    Finish Bluetooth: W2, after L2, 10d

    section API
    Notifications API Polyfill: API0, 2023-02, 4d
    TypeScript definitions: API1, after API0, 2d
```


[0]:https://github.com/socketsupply/io
