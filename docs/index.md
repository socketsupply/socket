# DOCS

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


