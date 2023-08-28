## Contribution Guide Overview
This guide provides steps on how to make contributions, report bugs, and become a part of the ongoing development of `Socket Runtime`.  
If you are new to Socket Runtime, the [Guides](https://socketsupply.co/guides/) is a good place to start.    

## Requirements
To contribute to the project you need to clone the repository and install the dependencies. 

#### MacOS and Linux

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

## Project directory structure

The project is structured as follows:  

1- [`api`](api/): The `api` folder contains the JavaScript API for Socket Runtime. 
   It consists of the built-in modules that are available in the runtime and the `socket` package that is published to npm (i.e. `socket:fs`, `socket:crypto`, etc.).    
   These modules have native bindings to the underlying C++/Objective-C/Kotlin code and libuv to expose the platform
   capabilities to the JavaScript code.  

2- [`bin`](bin/): This directory contains useful scripts for building the project on different platforms, managing versions,
   generating documentation, publishing npm packages, etc.  

3- [`npm`](npm/): This directory consists of the JavaScrip wrappers for the native code, build scripts and package directories.
   This directory consists of the JavaScrip wrappers for the native code, build scripts and package directories.
   You can also find the official Socket Runtime Node.js backend in the
   [`npm/packages/@socketsupply/socket-node`](npm/packages/%40socketsupply/socket-node/) directory.  

4- [`src`](src/): This directory contains the native code for the Socket Runtime:
  - [`cli`](src/cli/): contains the source code for the Socket Runtime CLI
  - [`core`](src/core/): contains the source code for the Socket Runtime core, such as Bluetooth support,
    File System, UDP, Peer-to-Peer capabilities, JavaScript bindings, etc.
  - [`desktop`](src/desktop/): contains the source code for the Socket Runtime library for desktop platforms
  - [`android`](src/android/): contains the source code for the Socket Runtime library for Android
  - [`ios`](src/ios/): contains the source code for the Socket Runtime library for iOS
  - [`ipc`](src/ipc/): contains the source code for the Socket Runtime IPC library
  - [`process`](src/process/): contains the source code for the process management
  - [`window`](src/window/): contains the source code for the window management on desktop platforms  

5- [`test`](test/): This directory contains the actual Socket Runtime application that is used for testing the native code and the JavaScript API. 

Source: https://github.com/socketsupply/socket

## Repositories
- [Socket](https://github.com/socketsupply/socket): This the main repository where we keep track of bug reports and issues.  
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
