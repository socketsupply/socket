
# Main Process
When your new program runs, it will forward its arguments to the
"main"/"back-end" process.

# Render Process


## Methods
The `system` object is a global (ie, `window.system`).

### system.setTitle(Options: Object)
Set the native title of the window.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  value: String
}
```

### system.setSize(Options: Object)
Set the size of the window (will show the window).

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  width: Number,
  height: Number
}
```

### system.setMenu(Options: Object)
Set the native menu for the app.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  value: String
}
```

### system.send(Options: Object)
Send an object to the backend process and await a promise.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  value: Object
}
```

### system.hide(Options: Object)
Hide the entire app.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number
}
```

### system.show(Options: Object)
Show the entire app.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number
}
```

### system.exit(Options: Object)
Quits the backend process and then quits the render process,
the exit code used is the final exit code to the OS.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  code: Number
}
```

### system.dialog(Options: Object)
Opens a native file open/save dialog.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  isSave: Bool,
  allowFiles: Bool,
  allowMultiple: Bool
}
```

### system.openExternal(Options: Object)
Opens a link in the user's default browser.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  value: String
}
```

### system.contextMenu(Options: Object) `Render Process Only`
Opens a native context menu.

```ts
@param {Object} Options
@returns Promise<void>

struct Options {
  window: Number,
  value: Object
}
```

## Properties
The `process` object is a global (ie, `window.process`).

### process.argv: Array<String>
Provides the arguments that the program was called with.

### process.title: String
The title as defined in the `operator.config` file.

### process.version: String
The version as defined in the `operator.config` file.

### process.debug: Number
Value is `1` unless `-xd` is passed to the CLI tool at build time.

### process.bundle: String
A value returned by the OS that represents the path to the running app.

### process.executable: String
The executable name as defined in the `operator.config` file.
