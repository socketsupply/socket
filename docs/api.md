# API
All methods work from either the Render or Main process.

## Methods

### await main.setTitle(title: String)
Set the native title of the window.

### await main.setSize({ width: Number, height: Number })
Set the size of the window (will show the window).

### await main.setMenu(value: String)
Set the native menu for the app.

### await main.send(value: Any)
Send an object to the backend process and await a promise.

### await main.hide()
Hide the entire app.

### await main.show()
Show the entire app.

### await main.quit(options: Object)
Quits the backend process and then quits the render process.

### await main.dialog(options: Object)
Opens a native file open/save dialog.

### await main.openExternal(url: String)
Opens a link in the user's default browser.

```js
try {
  await main.openExternal('https://google.com')
} catch (err) {
  ...
}
```

### await main.contextMenu(value: Object) `Render Process Only`
Opens a native context menu.

## Properties

### window.main.argv: Array<String>
Provides the arguments that the program was called with.

### window.main.name: String
The name as defined in the `settings.congig` file.

### window.main.verison: String
The version as defined in the `settings.config` file.

### window.main.debug: Number
Value is `1` unless `-xd` is passed to the CLI tool at build time.
