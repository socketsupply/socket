# Command Line interface
These commands are available from the command line interface (CLI).
# ssc

## Usage
```bash
ssc [SUBCOMMAND] [options] [<project-dir>]
ssc [SUBCOMMAND] -h
```
## subcommands
| Option | Description |
| --- | --- |
| build | build project |
| list-devices | get the list of connected devices |
| init | create a new project (in the current directory) |
| install-app | install app to the device |
| print-build-dir | print build path to stdout |
| run | run application |
| env | print relavent environment variables |
| setup | install build dependencies |
## general options
| Option | Description |
| --- | --- |
| -h, --help | print help message |
| --prefix | print install path |
| -v, --version | print program version |
# ssc build
Build Socket application.
## Usage
```bash
ssc build [options] [<project-dir>]
```
## options
| Option | Description |
| --- | --- |
| --platform=<platform> | platform target to build application for (defaults to host):
- android
- android-emulator
- ios
- ios-simulator |
| --port=<port> | load "index.html" from "http://localhost:<port>" |
| --test[=path] | indicate test mode, optionally importing a test file relative to resource files |
| --headless | build application to run in headless mode (without frame or window) |
| --prod | build for production (disables debugging info, inspector, etc.) |
| --stdin | read from stdin (dispacted as 'process.stdin' event to window #0) |
| -D, --debug | enable debug mode |
| -o, --only-build | only run build step, |
| -p, --package | package the app for distribution |
| -q, --quiet | hint for less log output |
| -r, --run | run after building |
| -V, --verbose | enable verbose output |
| -w, --watch | watch for changes to rerun build step |
## Linux options
| Option | Description |
| --- | --- |
| -f, --package-format=<format> | package a Linux application in a specified format for distribution:
- deb (default)
- zip |
## macOS options
| Option | Description |
| --- | --- |
| -c | code sign application with 'codesign' |
| -n | notarize application with 'notarytool' |
| -f, --package-format=<format> | package a macOS application in a specified format for distribution:
- zip (default)
- pkg |
## iOS options
| Option | Description |
| --- | --- |
| -c | code sign application during xcoddbuild
(requires '[ios] provisioning_profile' in 'socket.ini') |
## Windows options
| Option | Description |
| --- | --- |
| -f, --package-format=<format> | package a Windows application in a specified format for distribution:
- appx (default) |
# ssc list-devices
Get the list of connected devices.
## Usage
```bash
ssc list-devices [options] --platform=<platform>
```
## options
| Option | Description |
| --- | --- |
| --platform=<platform> | platform target to list devices for:
- android
- ios |
| --ecid | show device ECID (ios only) |
| --udid | show device UDID (ios only) |
| --only | only show ECID or UDID of the first device (ios only) |
# ssc init
Create a new project. If the path is not provided, the new project will be created in the current directory.
## Usage
```bash
ssc init [<project-dir>]
```
## options
| Option | Description |
| --- | --- |
| -C, --config | only create the config file |
| -n, --name | project name |
# ssc install-app
Install the app to the device or host target.
## Usage
```bash
ssc install-app [--platform=<platform>] [--device=<identifier>] [options]
```
## options
| Option | Description |
| --- | --- |
| -D, --debug | enable debug output |
| --device[=identifier] | identifier (ecid, ID) of the device to install to
if not specified, tries to run on the current device |
| --platform=<platform> | platform to install application to device (defaults to host)::
- android
- ios |
| --prod | install production application |
| -V, --verbose | enable verbose output |
## macOS options
| Option | Description |
| --- | --- |
| --target=<target> | installation target for macOS application (defaults to '/')
the application is installed into '$target/Applications' |
# ssc print-build-dir
Create a new project (in the current directory)
## Usage
```bash
ssc print-build-dir [--platform=<platform>] [--prod] [--root] [<project-dir>]
```
## options
| Option | Description |
| --- | --- |
| --platform | platform to print build directory for (defaults to host):
- android
- android-emulator
- ios
- ios-simulator |
| --prod | indicate production build directory |
| --root | print the root build directory |
# ssc run
Run application.
## Usage
```bash
ssc run [options] [<project-dir>]
```
## options
| Option | Description |
| --- | --- |
| -D, --debug | enable debug mode |
| --headless | run application in headless mode (without frame or window) |
| --platform=<platform> | platform target to run application on (defaults to host):
- android
- android-emulator
- ios
- ios-simulator |
| --prod | build for production (disables debugging info, inspector, etc.) |
| --test[=path] | indicate test mode, optionally importing a test file relative to resource files |
| -V, --verbose | enable verbose output |
# ssc setup
Setup build tools for host or target platform.
Platforms not listed below can be setup using instructions at https://socketsupply.co/guides
## Usage
```bash
ssc setup [options] --platform=<platform> [-y|--yes]
```
## options
| Option | Description |
| --- | --- |
| --platform=<platform> | platform target to run setup for (defaults to host):
- android
- ios |
| -q, --quiet | hint for less log output |
| -y, --yes | answer yes to any prompts |
