# Menus

## System Menus

System menus are created at build time. Your project needs a `menu.config`
in the root folder. The semi colon is significant indicates the end of the
menu. Use an underscore when there is no accellerator key.

```syntax
Edit:
  Cut: <CtrlOrMeta>+X
  Copy: <CtrlOrMeta>+C
  Paste: <CtrlOrMeta>+V
  Delete: _
  Select All: <CtrlOrMeta>+A;

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


## Context Menus

Dynamically build a context menu and await the user's selection.

```js
const value = await contextMenu({
  Download: 'd',
  Wizard: 'w',
  Share: 's'
})
```

If the user presses `w` or clicks the `Wizard` menu item, the following
statement will evalute true.

```js
assert(value === { title: 'Wizard', parent: 'contextMenu', state: 0 })
```
