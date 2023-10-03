## Contribution Guide Overview
This guide provides steps on how to make contributions, report bugs, and become a part of the ongoing development of `Socket Runtime`.
If you are new to Socket Runtime, the [Guides](https://socketsupply.co/guides/) is a good place to start.

## Requirements
To contribute to the project you need to clone the repository and install the dependencies.

### MacOS and Linux

```bash
git clone git@github.com:socketsupply/socket.git
cd socket
./bin/install.sh
```
### Windows

```powershell
git clone git@github.com:socketsupply/socket.git
cd socket
.\bin\install.ps1
```

## Building `ssc` and other useful commands

`ssc` (short for Socket Supply Compiler) is a command-line tool that is used to build Socket Runtime applications.
It is a wrapper around the native build tools for each platform and provides a unified interface for building Socket Runtime applications. You don't have to use Xcode, MSAndroid Studio, or Visual Studio to build Socket Runtime applications, but you can if you want to.

### `./bin/install.sh` (macOS and Linux) or `.\bin\install.ps1` (Windows)

This command installs the dependencies for Socket Runtime and builds `ssc` for your platform.
- `ssc` on macOS can cross-compile for iOS, iOS Simulator, Android, and Android Emulator.
- `ssc` on Linux can cross-compile for Android and Android Emulator.
- `ssc` on Windows can cross-compile for Android and Android Emulator.

Additional flags on macOS and Linux:
- `VERBOSE=1` - prints useful information about the build process. Please use this flag if you are reporting a bug.
- `DEBUG=1` - builds `ssc` in debug mode. This is useful if you are developing `ssc` and want to debug it.
- `NO_ANDROID=1` - skips the `ssc`'s Android support. This is useful if you are not developing for Android and don't want to install the Android SDK.
- `NO_IOS=1` - skips the `ssc`'s iOS support. This is useful if you are not developing for iOS and don't want to install the iOS SDK. It's only useful on macOS.
- `CPU_CORES={number}` - sets the number of CPU cores to use for the build. This is useful if you want to speed up the build process. The default value is the number of CPU cores on your machine.
<!--
TODO(@chicoxyzzy):
- `SOCKET_HOME={path}` - sets the path to the Socket Runtime build directory. This is useful if you want to build Socket Runtime in a different directory than the default one.

what else?
-->

Additional flags on Windows:
- `-debug` - builds `ssc` in debug mode. This is useful if you are developing `ssc` and want to debug it.
- `-verbose` - prints useful information about the build process. Please use this flag if you are reporting a bug.
- `-yesdeps` - automatically installs the dependencies for Socket Runtime.

### `./bin/clean.sh` (macOS and Linux)

Sometimes you need to clean the build artifacts and start from scratch. This command removes the build artifacts for `ssc` and the Socket Runtime libraries.

## Project directory structure

The project is structured as follows:

1. [`api`](https://github.com/socketsupply/socket/tree/master/api): The `api` folder contains the JavaScript API for Socket Runtime.
It consists of the built-in modules that are available in the runtime and the `socket` package that is published to npm (i.e. `socket:fs`, `socket:crypto`, etc.).
These modules have native bindings to the underlying C++/Objective-C/Kotlin code and libuv to expose the platform
capabilities to the JavaScript code.

1. [`bin`](https://github.com/socketsupply/socket/tree/master/bin): This directory contains useful scripts for building the project on different platforms, managing versions, generating documentation, publishing npm packages, etc.

1. [`npm`](https://github.com/socketsupply/socket/tree/master/npm): This directory consists of the JavaScrip wrappers for the native code, build scripts and package directories.
This directory consists of the JavaScrip wrappers for the native code, build scripts and package directories.
You can also find the official Socket Runtime Node.js backend in the
[`npm/packages/@socketsupply/socket-node`](https://github.com/socketsupply/socket/tree/master/npm/packages/%40socketsupply/socket-node) directory.

1. [`src`](https://github.com/socketsupply/socket/tree/master/src): This directory contains the native code for the Socket Runtime:
- [`android`](https://github.com/socketsupply/socket/tree/master/src/android): contains the source code for the Socket Runtime library for Android
- [`app`](https://github.com/socketsupply/socket/tree/master/src/app): contains the source code related to the Socket Runtime application instance
- [`cli`](https://github.com/socketsupply/socket/tree/master/src/cli): contains the source code for the Socket Runtime CLI
- [`core`](https://github.com/socketsupply/socket/tree/master/src/core): contains the source code for the Socket Runtime core, such as Bluetooth support, File System, UDP, Peer-to-Peer capabilities, JavaScript bindings, etc.
- [`desktop`](https://github.com/socketsupply/socket/tree/master/src/desktop): contains the source code for the Socket Runtime library for desktop platforms
- [`extension`](https://github.com/socketsupply/socket/tree/master/src/extension): contains the source code for the Socket Runtime extensions ABI
- [`ios`](https://github.com/socketsupply/socket/tree/master/src/ios): contains the source code for the Socket Runtime library for iOS
- [`ipc`](https://github.com/socketsupply/socket/tree/master/src/ipc): contains the source code for the Socket Runtime IPC library
- [`process`](https://github.com/socketsupply/socket/tree/master/src/process): contains the source code for the process management
- [`window`](https://github.com/socketsupply/socket/tree/master/src/window): contains the source code for the window management on desktop platforms

1. [`test`](https://github.com/socketsupply/socket/tree/master/src/test): This directory contains the actual Socket Runtime application that is used for testing the native code and the JavaScript API.

## Building Socket Runtime applications

Once you have built `ssc`, you can use it to build Socket Runtime applications.
`ssc -h` command prints the help message for `ssc` and lists all the available commands.

Check out the [Guides](https://socketsupply.co/guides/) for more information on how to build Socket Runtime applications.

## Adding tests

We run Socket Runtime E2E tests from the actual Socket Runtime application located in the [`test`](https://github.com/socketsupply/socket/tree/master/src/test) directory. This allows us to test the Socket Runtime API from the perspective of the Socket Runtime application developer. The tests are written in JavaScript and use the built-in `socket:test` module to test the Socket Runtime API. `socket:test` is a port of the [`tapzero`](https://github.com/socketsupply/tapzero) testing framework.

To run the tests, use the following command:

```bash
npm run test
```
You can also run specific tests against specific module by using the `--test` flag:

```bash
ssc run --test=application.js ./test
```
Be sure to build `ssc` before running the tests if you have made any changes to the Socket Runtime source code.
You can also rebuild and run `ssc` with a single command:

```bash
ssc build -r --test=application.js ./test
```

## Other repositories
- [Socket-Examples](https://github.com/socketsupply/socket-examples): This repository contains example projects powered by Socket which helps you build cross-platform apps for desktop and mobile.
- [Create-socket-app](https://github.com/socketsupply/create-socket-app): This repository will help you build native apps for mobile and desktop with Svelte, Reactjs, Vuejs, and others, by providing a few basic boilerplates and some strong opinions so you can get coding on a production-quality app as quickly as possible.

## Creating an issue
Issues for all Socket Runtime components are on [GitHub](https://github.com/socketsupply/socket). When reporting issues, please follow these guidelines:
- If you think you have found a bug, Please [open an issue](https://github.com/socketsupply/socket/issues/new) and make sure that you select the correct template and follow the given instructions while creating an issue.
- You can search through existing issues to see if there is a similar one [reported](https://github.com/socketsupply/socket/issues). You can also search through closed issues as they may have been closed with a solution.
- Please detail all the steps necessary to reproduce the issue you are running into, so we can easily diagnose and fix your issue.
- If you have found a potential security issue, please use the GitHub Security Advisory [Report a Vulnerability](https://github.com/socketsupply/socket/security/advisories/new) tab to report the issue. We will work with you the the entire lifecycle of the issue from the time you report it, to the time it has remediation, and is ready for an announcement.

If you can't find something in this guide or you have any questions, please feel free to join our [Discord](https://discord.com/invite/YPV32gKCsH)

## Claiming Issues
If you find a bug or an issue that you would like to work on, you can ask to claim it, please leave a comment indicating your intention and we will assign it to you. If an issue is not being worked on, feel free to work on it yourself (but please comment first to let us know about your attention to work on it so the assignee knows).

## Making Pull requests
Before submitting a pull request, please follow these steps:
- [Create an issue](https://github.com/socketsupply/socket/issues/new) if one has not already been created.
- Call out a reviewer to look at and merge your code, this helps our team to keep track of the pull requests and make sure there isn't duplicated effort.

Thanks for helping to improve Socket Runtime!
