import { test } from 'socket:test'
import { primordials } from 'socket:ipc'
import application from 'socket:application'
import { ApplicationWindow } from 'socket:window'

//
// Polyfills
//
test('window.resizeTo', async (t) => {
  window.resizeTo(420, 200)
  const mainWindow = await application.getCurrentWindow()
  const { width, height } = mainWindow.getSize()
  t.equal(width, 420, 'width is 420')
  t.equal(height, 200, 'height is 200')
})

// this one is not in the spec
test('window.resizeTo percentage', async (t) => {
  window.resizeTo('20%', '20%')
  const mainWindow = await application.getCurrentWindow()
  const { width, height } = mainWindow.getSize()
  t.equal(width, Math.ceil(window.screen.width * 0.2), 'width is 420')
  t.equal(height, Math.ceil(window.screen.height * 0.2), 'height is 200')
})

test('window.document.title', async (t) => {
  window.document.title = 'idkfa'
  t.equal(window.document.title, 'idkfa', 'window.document.title is has been changed')
  const mainWindow = await application.getCurrentWindow()
  t.equal(mainWindow.getTitle(), 'idkfa', 'window title is correct')
})
//
// End of polyfills
//

test('version', (t) => {
  t.equal(application.version.short, primordials.version.short, 'short version is correct')
  t.equal(application.version.hash, primordials.version.hash, 'version hash is correct')
  t.equal(application.version.full, primordials.version.full, 'full version is correct')
})

test('getScreenSize', async (t) => {
  const { width, height } = await application.getScreenSize()
  t.equal(width, window.screen.width, 'width is correct')
  t.equal(height, window.screen.height, 'height is correct')
})

test('createWindow without path', async (t) => {
  let err
  let dummyWindow
  try {
    dummyWindow = await application.createWindow({ index: 1 })
  } catch (e) {
    err = e
  }
  t.ok(err instanceof Error, 'throws error when path is not specified')
  t.equal(err.message, 'Path must be provided', 'error message is correct')
  t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
})

test('createWindow with invalid path', async (t) => {
  let err
  let dummyWindow
  try {
    dummyWindow = await application.createWindow({ index: 1, path: 'invalid.path' })
  } catch (e) {
    err = e
  }
  t.ok(err instanceof Error, 'throws error when path is invalid')
  t.ok(err.message.startsWith('Error: only .html files are allowed. Got url file://'), 'error message is correct')
  t.ok(err.message.endsWith('invalid.path'), 'error shows correct path')
  t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
})

test('createWindow with non-existent path', async (t) => {
  let err
  let dummyWindow
  try {
    dummyWindow = await application.createWindow({ index: 1, path: 'invalid.html' })
  } catch (e) {
    err = e
  }
  t.ok(err instanceof Error, 'throws error when file does not exist')
  t.ok(err.message.startsWith('Error: file does not exist. Got url file://'), 'error message is correct')
  t.ok(err.message.endsWith('invalid.html'), 'error shows correct path')
  t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
})

test('createWindow with relative path', async (t) => {
  let err
  let dummyWindow
  try {
    dummyWindow = await application.createWindow({ index: 1, path: '../invalid.html' })
  } catch (e) {
    err = e
  }
  t.ok(err instanceof Error, 'throws error when file does not exist')
  t.ok(err.message.startsWith('Error: relative urls are not allowed. Got url file://'), 'error message is correct')
  t.ok(err.message.endsWith('invalid.html'), 'error shows correct path')
  t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
})

test('createWindow with existing index', async (t) => {
  let err
  let dummyWindow
  try {
    dummyWindow = await application.createWindow({ index: 0, path: 'index_no_js.html' })
  } catch (e) {
    err = e
  }

  t.ok(err instanceof Error, 'throws error when index is already used')
  t.equal(err.message, 'Error: Window with index 0 already exists', 'error message is correct')
  t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
})

test('createWindow with invalid dimensions', async (t) => {
  // passing Symbol() will throw on encodeURIComponent
  const sizes = [-1, '42', '-1%', '1.5px', '100.5%', 1n, {}, [], () => {}, true, false]
  const dimensions = ['width', 'height']
  for (const size of sizes) {
    for (const dimension of dimensions) {
      let err
      let dummyWindow
      try {
        dummyWindow = await application.createWindow({ index: 1, path: 'index_no_js.html', [dimension]: size })
      } catch (e) {
        err = e
      }
      let printValue
      switch (typeof size) {
        case 'bigint':
          printValue = `${size}n`
          break
        case 'function':
          printValue = '() => {}'
          break
        default:
          printValue = JSON.stringify(size)
      }
      t.ok(err instanceof Error, `throws error when ${dimension} is invalid (${printValue})`)
      t.equal(err.message, `Window ${dimension} must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${size} instead.`, `error message is correct for ${printValue}`)
      t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
    }
  }
})

test('getWindow with wrong options', async (t) => {
  // passing Symbol() will throw on encodeURIComponent`
  const options = [-1, 3.14, NaN, Infinity, -Infinity, 'IDDQD', 1n, {}, [], () => {}, true, false]
  for (const option of options) {
    let err
    let dummyWindow
    try {
      dummyWindow = await application.getWindow(option)
    } catch (e) {
      err = e
    }
    let printValue
    switch (typeof option) {
      case 'bigint':
        printValue = `${option}n`
        break
      case 'function':
        printValue = '() => {}'
        break
      default:
        printValue = JSON.stringify(option)
    }
    t.ok(err instanceof Error, `throws error when type ${printValue} is being passed`)
    t.equal(err.message, `Invalid window index: ${option} (must be a positive integer number)`, `error message is correct for ${printValue}`)
    t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
  }
})

test('getWindows with wrong options', async (t) => {
  // passing Symbol() will throw on encodeURIComponent`
  const options = [-1, 3.14, NaN, Infinity, -Infinity, 'IDDQD', 1n, {}, [], () => {}, true, false]
  for (const option of options) {
    let err
    let windows
    try {
      windows = await application.getWindows([option])
    } catch (e) {
      err = e
    }
    let printValue
    switch (typeof option) {
      case 'bigint':
        printValue = `${option}n`
        break
      case 'function':
        printValue = '() => {}'
        break
      default:
        printValue = JSON.stringify(option)
    }
    t.ok(err instanceof Error, `throws error when type ${printValue} is being passed`)
    t.equal(err.message, `Invalid window index: ${option} (must be a positive integer number)`, `error message is correct for ${printValue}`)
    t.equal(windows, undefined, 'does not return an ApplicationWindow instance')
  }
})

test('getWindows with non-array', async (t) => {
  // passing Symbol() will throw on encodeURIComponent`
  const options = [-1, 3.14, NaN, Infinity, -Infinity, 'IDDQD', 1n, {}, () => {}, true, false]
  for (const option of options) {
    let err
    let windows
    try {
      windows = await application.getWindows(option)
    } catch (e) {
      err = e
    }
    let printValue
    switch (typeof option) {
      case 'bigint':
        printValue = `${option}n`
        break
      case 'function':
        printValue = '() => {}'
        break
      default:
        printValue = JSON.stringify(option)
    }
    t.ok(err instanceof Error, `throws error when type ${printValue} is being passed`)
    t.equal(err.message, 'Indices list must be an array of integer numbers', `error message is correct for ${printValue}`)
    t.equal(windows, undefined, 'does not return an ApplicationWindow instance')
  }
})

test('getWindow with valid index', async (t) => {
  const mainWindow = await application.getWindow(0)
  t.ok(mainWindow instanceof ApplicationWindow, 'returns an ApplicationWindow instance')
  t.equal(mainWindow.index, 0, 'window index is correct')
})

test('getWindows with valid index', async (t) => {
  const windows = await application.getWindows([0])
  t.ok(windows instanceof Object, 'returns an object')
  t.ok(Object.keys(windows).every(index => Number.isInteger(Number(index))), 'keys are all integers')
  t.ok(Object.values(windows).every((window) => window instanceof ApplicationWindow), 'values are all ApplicationWindow instances')
})

// TODO(@chicoxyzzy): test other windows
test('getCurrentWindow', async (t) => {
  const mainWindow = await application.getCurrentWindow()
  t.ok(mainWindow instanceof ApplicationWindow, 'returns an ApplicationWindow instance')
  t.equal(mainWindow.index, 0, 'window index is correct')
})

// TODO(@chicoxyzzy): neither kill nor exit work so I use the counter workaround
let counter = 1

// TODO(@chicoxyzzy): fix incorrect sizes
test.skip('new window inherts the size of the main window when sizes are not provided', async (t) => {
  const mainWindow = await application.getWindow(0)
  const newWindow = await application.createWindow({ index: counter, path: 'index_no_js.html' })
  t.equal(mainWindow.index, 0, 'main window index is 0')
  t.equal(newWindow.index, counter, 'new window index is correct')
  const mainWindowSize = mainWindow.getSize()
  const newWindowSize = newWindow.getSize()
  t.equal(newWindowSize.width, mainWindowSize.width, 'width is inherited from the main window')
  t.equal(newWindowSize.height, mainWindowSize.height, 'height is inherited from the main window')
  // TODO(@chicoxyzzy): await newWindow.kill()
  await newWindow.close()
  counter++
})

// TODO(@chicoxyzzy): fix incorrect sizes`
test.skip('new window have the correct size when sizes are provided', async (t) => {
  const newWindow = await application.createWindow({
    index: counter,
    path: 'index_no_js.html',
    width: 800,
    height: 600
  })
  t.equal(newWindow.index, counter, 'new window index is correct')
  const newWindowSize = newWindow.getSize()
  t.equal(newWindowSize.width, 800, 'width is inherited from the main window')
  t.equal(newWindowSize.height, 600, 'height is inherited from the main window')
  // TODO(@chicoxyzzy): await newWindow.kill()
  await newWindow.close()
  counter++
})

// TODO(@chicoxyzzy): fix incorrect sizes
test.skip('new window have the correct size when sizes are provided in percent', async (t) => {
  const newWindow = await application.createWindow({
    index: counter,
    path: 'index_no_js.html',
    width: '100%',
    height: '50%'
  })
  t.equal(newWindow.index, counter, 'new window index is correct')
  const mainWindowSize = await application.getCurrentWindow().getSize()
  const newWindowSize = await newWindow.getSize()
  t.equal(newWindowSize.width, mainWindowSize.width, 'width is inherited from the main window')
  t.equal(newWindowSize.height, mainWindowSize.height / 2, 'height is inherited from the main window')
  // TODO(@chicoxyzzy): await newWindow.kill()
  await newWindow.close()
  counter++
})

test('getWindows all windows', async (t) => {
  const windows = await application.getWindows()
  t.ok(windows instanceof Object, 'returns an object')
  t.equal(Object.keys(windows).length, counter, 'returns all windows')
  t.ok(Object.keys(windows).every(index => Number.isInteger(Number(index))), 'keys are all integers')
  t.ok(Object.values(windows).every((window) => window instanceof ApplicationWindow), 'values are all ApplicationWindow instances')
})

test('setTitle', async (t) => {
  const mainWindow = await application.getCurrentWindow()
  const { title } = await mainWindow.setTitle('👋')
  t.equal(title, '👋', 'correct title is returned')
  t.equal(mainWindow.getTitle(), '👋', 'window options are updated')
})

test('hide / show', async (t) => {
  const mainWindow = await application.getCurrentWindow()
  const { status: statusHidden } = await mainWindow.hide()
  t.equal(statusHidden, ApplicationWindow.constants.WINDOW_HIDDEN, 'correct status is returned on hide')
  t.equal(mainWindow.getStatus(), ApplicationWindow.constants.WINDOW_HIDDEN, 'window options are updated on hide')
  const { status: statusShown } = await mainWindow.show()
  t.equal(statusShown, ApplicationWindow.constants.WINDOW_SHOWN, 'correct status is returned on show')
  t.equal(mainWindow.getStatus(), ApplicationWindow.constants.WINDOW_SHOWN, 'window options are updated on show')
})

// TODO(@chicoxyzzy): should navigation of main window throw? should navigation of current window throw? should we even allow navigation?
test.only('navigate window', async (t) => {
  const newWindow = await application.createWindow({ index: counter, path: 'index_no_js.html' })
  const { index, status } = await newWindow.navigate('index_no_js2.html')
  t.equal(index, newWindow.index, 'correct index is returned')
  t.equal(status, ApplicationWindow.constants.WINDOW_NAVIGATED, 'correct status is returned')
  counter++
})

// await new Promise((resolve) => {})
