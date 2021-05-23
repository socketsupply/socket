# Window
All global functions that are exposed by Opkit in the browser.

#### await window.setTitle(title: string)
Set the native title of the window.

#### await window.send(value: object)
Send an object to the backend process and await a promise.

#### await window.quit(value: object)
Quits the backend process and then quits the render process.

#### await window.dialog(value: object)
Opens a native file open/save dialog.

#### await window.contextMenu(value: object)
Opens a native context menu.
