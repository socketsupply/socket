## Getting started

### Requirements

To contribute to the project you need to clone the repository and install the dependencies. You can do this by running the following commands:

macOS or Linux:
```bash
git clone git@github.com:socketsupply/socket.git
cd socket
./scripts/install.sh
```

Windows:
```powershell
git clone git@github.com:socketsupply/socket.git
cd socket
.\scripts\install.ps1
```

Please follow the instructions in the terminal to install pre-requisites and add `ssc` to your path.

### Project structure

The project is structured as follows:
- [`api`](api/): contains the JavaScript API for Socket Runtime
- [`bin`](bin/): contains the useful scripts for building and maintaining the project
- [`npm`](npm/): contains the source code for the npm packages
- [`src`](src/): contains the source code for the native part of the project
- [`test`](test/): contains the tests for the project

#### `api`

The `api` folder contains the JavaScript API for Socket Runtime. It consists of the built-in modules that are available in the runtime and the `socket` package that is published to npm (i.e. `socket:fs`, `socket:crypto`, etc.).
These modules have native bindings to the underlying C++/Objective-C/Kotlin code and libuv to expose the platform
capabilities to the JavaScript code.

#### `bin`

TODO

#### `npm`

This directory consists of the JavaScrip wrappers for the native code, build scripts and the package directories.
You can also find the official Socket Runtime Node.js backend in the
[`npm/packages/@socketsupply/socket-node`](npm/packages/%40socketsupply/socket-node/) directory.

#### `src`

TODO

#### `test`

This directory contains the actual Socket Runtime application that is used for testing the native code and the JavaScript API.
