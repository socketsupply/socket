# DOCS

## Building an app

The build tool expects to find two config files in the
target directory, `settings.config` and `menu.config`.

### Settings

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

### Menus

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
in the root folder. The semi colon is significant indicates the end of the menu.

```js
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

When a selection is made the `menuItemSelected` event will be fired in the
browser. It will contain the same info as the result from the context menu.

## IPC

#### From the browser

```js
const result = await send('honk')
assert(result === 'goose')
```

#### In the backend process (node.js example)

```js
import { send, receive } from './ipc.js'

receive(async data => {
  if (data === 'honk') send('goose')
})
```


