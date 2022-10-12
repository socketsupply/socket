# Socket Runtime

Socket is a client-side runtime for creating native cross-platform apps for
mobile and desktop using HTML, CSS, and JavaScript. It also exposes primitives
needed for building peer-to-peer and local-first applications, such as Bluetooth,
UDP and File I/O.

> [![Socket SDK CI](https://github.com/socketsupply/socket/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/socketsupply/socket/actions/workflows/ci.yml)

Read the [docs](https://sockets.sh)

```mermaid
gantt
    title Project Roadmap
    dateFormat  YYYY-MM-DD

    section APIs
    Notifications API Polyfil: API0, 2d
    TypeScript definitions: API1

    section All
    Cross Compiling: X0, 2022-10-12, 20d
    Consolidate Routing: X1, 2022-10-8, 6d

    section Android
    Finish Bluetooth: A1, after X1, 14d

    section iOS
    Security Crash Messaging: I0, 2022-12, 2d

    section MacOS
    Security Crash Messaging: M0, 2022-12, 2d

    section Linux
    Finish Bluetooth: L2, after A1, 14d

    section Windows
    Core Integration: W0, after B1, 20d
    Finish Bluetooth: W2, after L2, 10d
```

