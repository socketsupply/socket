# Window
Opkit exposes a global named `main`, which is where all the
ipc methods are located.

#### await main.setTitle(title: string)
Set the native title of the window.

#### await main.setSize({ width: 0, height: 0 })
Set the size of the window (will show the window).

#### await main.setMenu(value: string)
Set the native menu for the app.

#### await main.send(value: object)
Send an object to the backend process and await a promise.

#### await main.hide()
Hide the entire app.

#### await main.show()
Show the entire app.

#### await main.quit(value: object)
Quits the backend process and then quits the render process.

#### await main.dialog(value: object)
Opens a native file open/save dialog.

#### await main.contextMenu(value: object)
Opens a native context menu.
