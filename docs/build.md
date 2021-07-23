# Building apps

Opkit provides a cli that can build, package and code-sign for
`win32`, `macOS` and `linux`.

## Command

The CLI can be installed globally with the `bootstrap.sh` script.
The script should install a binary called `opkit` which is placed
in `/usr/local/bin` or `C:\Program Files`, so it should be in your
path. Run the command with the `-h` flag to get help.

```sh
opkit -h
```

If the CLI tool is failing, see the [trouble shooting guide][0].

## Configuration Files

The build tool expects to find a config file in the
target directory, `settings.config`.

### Settings Configuration

`input` is a directory is where your application's code
is located. The `build` string is shell code that the CLI
tool will run for you.

```syntax
#
# Build Settings
#
input: src
build: node build.js
output: dist
executable: operator

#
# Package Metadata
#
version: v0.0.1
versionShort: 0.0
revision: 1
name: Operator
description: A demo appliation
copyRight: Operator Tool Co. © 2021-2022
maintainer: Operator Contributors <dev@optool.co>
mac_category: Developer Tools
linux_categories: Developer
mac_bundle_identifier: co.optool.demo

#
# Code Signing
#
mac_sign: Voltra Co. BV (DYE7429KTV)
win32_sign: DigiCert: Operator Tool Co.

#
# window
#
title: Operator
width: 750
height: 520

#
# Backend
#
cmd: node
arg: main.js

#
# Advanced Compiler Settings
#
flags: -O3
arch: x64
```

# PLATFORM SPECIFICS

## MAC
You need to copy a entitlements.plist file to the build target directory if you
specify the `-me` flag.

[0]:/docs/troubleshooting.md
