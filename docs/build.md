# Building apps

Unlike electron, there is an official build tool built into
Opkit. It can build, package and code-sign for all 3 major
operating systems.

## Command

The build tool can be installed globally with the `bootstrap.sh`
script. The script should install a binary called `opkit` which
is placed in `/usr/local/bin` or `C:\Program Files`, so it should
be in your path. Run the command with the `-h` flag to get help.

```sh
opkit -h
```

## Configuration Files

The build tool expects to find two config files in the
target directory, `settings.config` and `menu.config`.

### Settings Configuration

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
mac_category: Developer Tools
linux_categories: Developer
maintainer: Operator Contributors <dev@optool.co>

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

### Menu Configuration Syntax

End menus with `;`. Split menus and menu items with `:`. See the
docs for an example of a menu.

```syntax
Menu A:
  Item 1: Accellerator
  Item 2: Accellerator
  Item 2: Accellerator;
Menu B:
  Item: Accellerator;
Menu C:
  Menu Item: Accellerator
```

