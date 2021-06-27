
# Main Process
When the main process is started, it is forwarded the arguments
passed to the built binary. The location from which the main
process is run will reveal its location on the file system.

All methods, unless indicated, can also be run in the main process.

# Render Process

## Methods
The `system` object is a global (ie, `window.system`).

### await system.setTitle(Options)
```ts
Options = {
  window: Number,
  value: String
}
```
Set the native title of the window.

### await system.setSize(Options)
```ts
Options = {
  window: Number,
  width: Number,
  height: Number
}
```
Set the size of the window (will show the window).

### await system.setMenu({ window: Number, value: String })
Set the native menu for the app.

### await system.send({ window: Number, value: Object })
Send an object to the backend process and await a promise.

### await system.hide({ window: Number })
Hide the entire app.

### await system.show({ window: Number })
Show the entire app.

### await system.exit(Options)
```ts
Options = {
  window: Number,
  code: Number
}
```

Quits the backend process and then quits the render process,
the exit code used is the final exit code to the OS.

### await system.dialog(Options)

```ts
Options = {
  window: Number,
  isSave: Bool,
  allowFiles: Bool,
  allowMultiple: Bool
}
```

Opens a native file open/save dialog.

### await system.openExternal(url: String)
Opens a link in the user's default browser.

```js
try {
  await system.openExternal('https://google.com')
} catch (err) {
  ...
}
```

### await system.contextMenu(value: Object) `Render Process Only`
Opens a native context menu.

## Properties
The `process` object is a global (ie, `window.process`).

### process.argv: Array<String>
Provides the arguments that the program was called with.

### process.title: String
The title as defined in the `settings.congig` file.

### process.verison: String
The version as defined in the `settings.config` file.

### process.debug: Number
Value is `1` unless `-xd` is passed to the CLI tool at build time.

### process.bundle: String
A value returned by the OS that represents the path to the running app.

### process.verison: String
The executable name as defined in the `settings.config` file.
