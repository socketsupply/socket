# DOCS

## Building an app

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

## Menus

#### Context Menus

Dynamically build a context menu and await the user's selection.

```js
const value = await contextMenu({
  'Download': 'd',
  'Wizard': 'w',
  'Share': 's'
})

assert(value === { title: 'Wizard', parent: 'contextMenu', state: 0 })
```

#### System Menus

System menus are created at build time. Your project needs a `menu.config`
in the root folder. The semi colon is significant indicates the end of the
menu.

```syntax
Edit:
  Cut: x
  Copy: c
  Paste: v
  Delete: _
  Select All: a;

Other:
  Apple: _
  Another Test: t
```

When a menu item is activated, it raises the `menuItemSelected` event in
the front end code, you can then communicate with your backend code if you
want from there.

For example, if the `Apple` item is selected from the `Other` menu...

```js
window.addEventListener('menuItemSelected', event => {
  assert(event.detail.parent === 'Other')
  assert(event.detail.title === 'Apple')
})
```

## IPC

#### From the browser

The browser has a global function named `send` that creates a promise
which can await a response from the child process.

```js
const result = await send('honk')
assert(result === 'goose')
```

#### In the backend process (node.js example)

The spawned process should send and receive json using a simple
incrementing counter. Here is an example [implementation][0].

```js
import { send, receive } from './ipc.js'

receive(async data => {
  if (data === 'honk') send('goose')
})
```

[0]:https://github.com/optoolco/opkit/blob/master/test/example/src/main/ipc.js
