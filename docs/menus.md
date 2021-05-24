# Menus

## System Menus

System menus are created at build time. Your project needs a `menu.config`
in the root folder. The semi colon is significant indicates the end of the
menu. Use an underscore when there is no accelerator key. Modifiers are
optional. For the edit menu, Opkit will figure out which accelerators to
use for you.

```syntax
Edit:
  Cut: x
  Copy: c
  Paste: v
  Delete: _
  Select All: a

Other:
  Apple: _
  Another Test: T
  Some Thing: S + Command
  Bazz: s + Command, Control, Option
```

Accelerator modifiers are used as visual indicators but don't have a
material impact as the actual key binding is done in the event listener.

A capital letter implies that the accelerator is modified by the Shift key.

Additional accelerators are `Command`, `Control`, `Option`, each separated
by commas. If one is not applicable for a platform, it will just be ignored.

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
  Download: 'D',
  Wizard: 'W',
  Share: 'S'
})
```

If the user presses `w` or clicks the `Wizard` menu item, the following
statement will evalute true.

```js
assert(value === { title: 'Wizard', parent: 'contextMenu', state: 0 })
```
