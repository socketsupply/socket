## Getting started
Thanks for helping to improvev`SSC`! This guide covers ways in which you can become a part of the ongoing development of `SSC`.  

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

This directory contains the useful scripts for building the project on different platforms, managing versions,
generating documentation, publishing npm packages, etc.

#### `npm`

This directory consists of the JavaScrip wrappers for the native code, build scripts and the package directories.
You can also find the official Socket Runtime Node.js backend in the
[`npm/packages/@socketsupply/socket-node`](npm/packages/%40socketsupply/socket-node/) directory.

#### `src`

The `src` directory contains the native code for the Socket Runtime:
- [`cli`](src/cli/): contains the source code for the Socket Runtime CLI
- [`core`](src/core/): contains the source code for the Socket Runtime core, such as Bluetooth support,
File System, UDP, Peer-to-Peer capabilities, JavaScript bindings, etc.
- [`desktop`](src/desktop/): contains the source code for the Socket Runtime library for desktop platforms
- [`android`](src/android/): contains the source code for the Socket Runtime library for Android
- [`ios`](src/ios/): contains the source code for the Socket Runtime library for iOS
- [`ipc`](src/ipc/): contains the source code for the Socket Runtime IPC library
- [`process`](src/process/): contains the source code for the process management
- [`window`](src/window/): contains the source code for the window management on desktop platforms

#### `test`

This directory contains the actual Socket Runtime application that is used for testing the native code and the JavaScript API.  

### Creating an issue
- If you think you have found a bug, Please [open an issue](https://github.com/socketsupply/socket/issues/new) and make sure that you select the correct template and follow the given instructions while creating an issue.
- You can search through existing issues to see if there is a similar one [reported](https://github.com/socketsupply/socket/issues). You can also search through closed issues as it may have been closed with a solution.
- Please detail all the steps necessary to reproduce the issue you are running into, so we can easily diagnose and fix your issue.
- If you can't find something on this guide or you have any questions, please feel free to join our [Discord](https://discord.com/invite/YPV32gKCsH)
