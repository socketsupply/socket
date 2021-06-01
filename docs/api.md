# System API
All methods work from either the Render or Main process.

## Methods

### await system.setTitle(title: String)
Set the native title of the window.

### await system.setSize({ width: Number, height: Number })
Set the size of the window (will show the window).

### await system.setMenu(value: String)
Set the native menu for the app.

### await system.send(value: Any)
Send an object to the backend process and await a promise.

### await system.hide()
Hide the entire app.

### await system.show()
Show the entire app.

### await system.quit(options: Object)
Quits the backend process and then quits the render process.

### await system.dialog(options: Object)
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

# Process API

## Properties

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

### process.config: String
A value returned by the OS that represents the path to the user's config directory.

### process.home: String
A value returned by the OS that represents the path to the user's home directory.

### process.temp: String
A value returned by the OS that represents the path to the user's temp directory.

### process.verison: String
The executable name as defined in the `settings.config` file.
