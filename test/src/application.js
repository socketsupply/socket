import { test } from 'socket:test'
import { primordials } from 'socket:ipc'
import application from 'socket:application'
import { ApplicationWindow } from 'socket:window'
import { readFile } from 'socket:fs/promises'
import process from 'socket:process'

// TODO(@chicoxyzzy): consider window borders
// const DELTA = 28

let title = 'Socket Runtime JavaScript Tests'

test('window.document.title', async (t) => {
  t.equal(window.document.title, title, 'window.document.title is correct')
  title = 'idkfa'
  window.document.title = title
  t.equal(window.document.title, title, 'window.document.title is has been changed')
  // FIXME: make it work on iOS/Android
  if (!['android', 'ios'].includes(process.platform)) {
    const mainWindow = await application.getCurrentWindow()
    t.equal(mainWindow.getTitle(), title, 'window title is correct')
  }
})
//
// End of polyfills
//

// FIXME: make it work on iOS
// For some reason, on iOS versions are empty (`" ()"` for the full version)
if (process.platform !== 'ios') {
  test('application.runtimeVersion', (t) => {
    t.equal(application.runtimeVersion.short, primordials.version.short, 'short version is correct')
    t.equal(application.runtimeVersion.hash, primordials.version.hash, 'version hash is correct')
    t.equal(application.runtimeVersion.full, primordials.version.full, 'full version is correct')
  })
}

if (!['android'].includes(process.platform)) {
  test('application.debug', async (t) => {
    t.equal(typeof application.debug, 'boolean', 'debug is a boolean')
    t.equal(application.debug, globalThis.__args.debug, 'debug is correct')
    t.throws(() => { application.debug = 1 }, 'debug is immutable')
  })
}

// FIXME: make it work on iOS/Android
if (!['android', 'ios'].includes(process.platform)) {
  test('application.config', async (t) => {
    const rawConfig = await readFile('socket.ini', 'utf8')
    let prefix = ''
    const lines = rawConfig.split('\n')
    const config = []
    for (let line of lines) {
      line = line.trim()
      if (line.length === 0 || line.startsWith(';')) continue
      if (line.startsWith('[') && line.endsWith(']')) {
        prefix = line.slice(1, -1)
        continue
      }
      let [key, value] = line.split('=')
      key = key.trim()
      value = value.trim().replace(/^"/, '').replace(/"$/, '')
      config.push([prefix.length === 0 ? key : prefix + '_' + key, value])
    }
    config.forEach(([key, value]) => {
      switch (key) {
        // boolean values
        case 'build_headless':
        case 'window_max_width':
        case 'window_max_height':
        case 'window_min_width':
        case 'window_min_height':
        case 'window_resizable':
        case 'window_frameless':
        case 'window_utility':
          t.equal(application.config[key].toString(), value, `application.config.${key} is correct`)
          break
        case 'build_name':
          t.ok(application.config[key].startsWith(value), `application.config.${key} is correct`)
          break
        case 'test-section_array[]':
          t.ok([1, 2, 3].map(String).includes(value), 'test-section_array values are correct')
          break
        case 'test-section_subsection_key':
          t.equal(application.config[key], 'value', 'test-section_subsection_key values are correct')
          break
        case '.subsection_key': // FIXME(@jwerle): INI parser above
          t.ok(value, 'value', 'test-section.subsection_key == value')
          break
        default:
          // skip array values until we have a valid INI parser in stdlib
          if (!key.endsWith('[]')) {
            console.log(application.config[key], key, value)
            t.equal(application.config[key], value, `application.config.${key} is correct`)
          }
      }
      t.throws(
        () => { application.config[key] = 0 },
        // eslint-disable-next-line prefer-regex-literals
        /(read\s?only property)|(not extensible)/,
        `application.config.${key} is read-only`
      )
    })
  })
}

// FIXME: make it work on iOS/Windows
if (!['android', 'ios', 'win32'].includes(process.platform)) {
  test('application.getScreenSize', async (t) => {
    const { width, height } = await application.getScreenSize()
    t.equal(width, window.screen.width, 'width is correct')
    t.equal(height, window.screen.height, 'height is correct')
  })
}

// FIXME: make it work on iOS/Android
if (!['android', 'ios'].includes(process.platform)) {
  test('openExternal', async (t) => {
    const currentWindow = await application.getCurrentWindow()
    t.equal(typeof currentWindow.openExternal, 'function', 'openExternal is a function')
    if (process.platform !== 'linux') {
      const result = await currentWindow.openExternal('https://1.1.1.1')
      // can't test results without browser
      t.ok(result?.data, 'succesfully completes')
    }
  })
}

test('apllication.exit', async (t) => {
  t.equal(typeof application.exit, 'function', 'exit is a function')
})

if (!['android', 'ios', 'win32'].includes(process.platform)) {
  test('application.createWindow without path', async (t) => {
    let err
    let dummyWindow
    try {
      dummyWindow = await application.createWindow({ index: 1 })
    } catch (e) {
      err = e
    }
    t.ok(err instanceof Error, 'throws error when path is not specified')
    t.equal(err?.message, 'Path and index are required options', 'error message is correct')
    t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
  })

  test('application.createWindow with invalid path', async (t) => {
    let err
    let dummyWindow
    try {
      dummyWindow = await application.createWindow({ index: 1, path: 'invalid.path' })
    } catch (e) {
      err = e
    }
    t.ok(err instanceof Error, 'throws error when path is invalid')
    t.ok(err?.message?.startsWith('only .html files are allowed. Got url file://'), 'error message is correct')
    t.ok(err?.message?.endsWith('invalid.path'), 'error shows correct path')
    t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
  })

  test('application.createWindow with non-existent path', async (t) => {
    let err
    let dummyWindow
    try {
      dummyWindow = await application.createWindow({ index: 1, path: 'invalid.html' })
    } catch (e) {
      err = e
    }
    t.ok(err instanceof Error, 'throws error when file does not exist')
    t.ok(err?.message?.startsWith('file does not exist. Got url file://'), 'error message is correct')
    t.ok(err?.message?.endsWith('invalid.html'), 'error shows correct path')
    t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
  })

  test('application.createWindow with relative path', async (t) => {
    let err
    let dummyWindow
    try {
      dummyWindow = await application.createWindow({ index: 1, path: '../invalid.html' })
    } catch (e) {
      err = e
    }
    t.ok(err instanceof Error, 'throws error when file does not exist')
    t.ok(err?.message?.startsWith('relative urls are not allowed. Got url file://'), 'error message is correct')
    t.ok(err?.message?.endsWith('invalid.html'), 'error shows correct path')
    t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
  })

  test('application.createWindow with existing index', async (t) => {
    let err
    let dummyWindow
    try {
      dummyWindow = await application.createWindow({ index: 0, path: 'index_no_js.html' })
    } catch (e) {
      err = e
    }

    t.ok(err instanceof Error, 'throws error when index is already used')
    t.equal(err?.message, 'Window with index 0 already exists', 'error message is correct')
    t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
  })

  test('application.createWindow with invalid dimensions', async (t) => {
    // passing Symbol() will throw on encodeURIComponent
    const sizes = [-1, '42', '-1%', '1.5px', '100.5%', 1n, {}, [], () => {}, true, false]
    const dimensions = ['width', 'height']
    for (const size of sizes) {
      for (const dimension of dimensions) {
        let err
        let dummyWindow
        try {
          dummyWindow = await application.createWindow({ index: 1, path: 'frontend/index_no_js.html', [dimension]: size })
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
        t.equal(err?.message, `Window ${dimension} must be an integer number or a string with a valid percentage value from 0 to 100 ending with %. Got ${size} instead.`, `error message is correct for ${printValue}`)
        t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
      }
    }
  })

  test('application.getWindow with wrong options', async (t) => {
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
      t.equal(err?.message, `Invalid window index: ${option} (must be a positive integer number)`, `error message is correct for ${printValue}`)
      t.ok(!(dummyWindow instanceof ApplicationWindow), 'does not return an ApplicationWindow instance')
    }
  })

  test('application.getWindows with wrong options', async (t) => {
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
      t.equal(err?.message, `Invalid window index: ${option} (must be a positive integer number)`, `error message is correct for ${printValue}`)
      t.equal(windows, undefined, 'does not return an ApplicationWindow instance')
    }
  })

  test('application.getWindows with non-array', async (t) => {
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
      t.equal(err?.message, 'Indices list must be an array of integer numbers', `error message is correct for ${printValue}`)
      t.equal(windows, undefined, 'does not return an ApplicationWindow instance')
    }
  })

  test('application.getWindow with valid index', async (t) => {
    const mainWindow = await application.getWindow(0)
    t.ok(mainWindow instanceof ApplicationWindow, 'returns an ApplicationWindow instance')
    t.equal(mainWindow.index, 0, 'window index is correct')
  })

  test('application.getWindows with valid indexes', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    counter++
    const windows = await application.getWindows([0, newWindow.index])
    t.ok(windows instanceof Object, 'returns an object')
    t.equal(Object.keys(windows).length, 2, 'object has 2 keys')
    t.ok(Object.keys(windows).every(index => Number.isInteger(Number(index))), 'keys are all integers')
    t.ok(Object.values(windows).every((window) => window instanceof ApplicationWindow), 'values are all ApplicationWindow instances')
    t.equal(windows[0].index, 0, 'window index is correct')
    t.equal(windows[newWindow.index].index, newWindow.index, 'window index is correct')
    newWindow.close()
  })

  test('application.getWindows without params', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    counter++
    const windows = await application.getWindows()
    t.ok(windows instanceof Object, 'returns an object')
    t.ok(Object.keys(windows).length === counter, `object has ${counter} windows`)
    t.ok(Object.keys(windows).every(index => Number.isInteger(Number(index))), 'keys are all integers')
    t.ok(Object.values(windows).every((window) => window instanceof ApplicationWindow), 'values are all ApplicationWindow instances')
    t.equal(windows[0].index, 0, 'window index is correct')
    t.equal(windows[newWindow.index].index, newWindow.index, 'window index is correct')
    newWindow.close()
  })

  test('application.getCurrentWindow', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    t.ok(mainWindow instanceof ApplicationWindow, 'returns an ApplicationWindow instance')
    t.equal(mainWindow.index, 0, 'window index is correct')
    const mainWindowTitle = mainWindow.getTitle()
    t.equal(mainWindowTitle, title, 'title is correct')
    const mainWindowStatus = mainWindow.getStatus()
    t.equal(mainWindowStatus, ApplicationWindow.constants.WINDOW_SHOWN, 'status is correct')
  })

  // TODO(@chicoxyzzy): neither kill nor exit work so I use the counter workaround
  let counter = 1

  test('window.close', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    counter++
    const { status } = await newWindow.close()
    t.equal(status, ApplicationWindow.constants.WINDOW_CLOSED, 'window is closed')
  })

  test('new window have the correct size when sizes are provided', async (t) => {
    const newWindow = await application.createWindow({
      index: counter,
      path: 'frontend/index_no_js.html',
      width: 800,
      height: 600
    })
    t.equal(newWindow.index, counter, 'new window index is correct')
    counter++
    const newWindowSize = newWindow.getSize()
    t.equal(newWindowSize.width, 800, 'width is inherited from the main window')
    // TODO(@chicoxyzzy): window borders
    // t.equal(newWindowSize.height, 600, 'height is inherited from the main window')
    // TODO(@chicoxyzzy): await newWindow.kill()
    await newWindow.close()
  })

  test('new window have the correct size when sizes are provided in percent', async (t) => {
    const newWindow = await application.createWindow({
      index: counter,
      path: 'frontend/index_no_js.html',
      width: '100%',
      height: '50%'
    })
    t.equal(newWindow.index, counter, 'new window index is correct')
    counter++
    const newWindowSize = await newWindow.getSize()
    t.equal(newWindowSize.width, window.screen.width, 'width is inherited from the main window')
    // t.equal(newWindowSize.height, window.screen.height / 2, 'height is inherited from the main window')
    // TODO(@chicoxyzzy): await newWindow.kill()
    await newWindow.close()
  })

  test('new window inherts the size of the main window when sizes are not provided', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    t.equal(newWindow.index, counter, 'new window index is correct')
    counter++
    const newWindowSize = newWindow.getSize()
    t.equal(newWindowSize.width, Math.round(window.screen.width * 0.8), 'width is inherited from the main window')
    // TODO(@chicoxyzzy): window borders
    // t.equal(newWindowSize.height, Math.round(window.screen.height * 0.8), 'height is inherited from the main window')
    // TODO(@chicoxyzzy): await newWindow.kill()
    await newWindow.close()
  })

  test('window.setTitle', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    const { title } = await mainWindow.setTitle('👋')
    t.equal(title, '👋', 'correct title is returned')
    t.equal(mainWindow.getTitle(), '👋', 'window options are updated')
  })

  test('window.setSize in pixels', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    const { width, height } = await mainWindow.setSize({ width: 800, height: 600 })
    t.equal(width, 800, 'correct width is returned')
    t.equal(height, 600, 'correct height is returned')
    t.equal(mainWindow.getSize().width, 800, 'window options are updated')
    t.equal(mainWindow.getSize().height, 600, 'window options are updated')
  })

  test('window.setSize in percent', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    const { width, height } = await mainWindow.setSize({ width: '50%', height: '50%' })
    t.equal(width, window.screen.width / 2, 'correct width is returned')
    t.equal(height, Math.floor(window.screen.height / 2), 'correct height is returned')
    t.equal(mainWindow.getSize().width, window.screen.width / 2, 'window options are updated')
    t.equal(mainWindow.getSize().height, Math.floor(window.screen.height / 2), 'window options are updated')
  })

  test('window.hide / window.show', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    const { status: statusHidden } = await mainWindow.hide()
    t.equal(statusHidden, ApplicationWindow.constants.WINDOW_HIDDEN, 'correct status is returned on hide')
    t.equal(mainWindow.getStatus(), ApplicationWindow.constants.WINDOW_HIDDEN, 'window options are updated on hide')
    const { status: statusShown } = await mainWindow.show()
    t.equal(statusShown, ApplicationWindow.constants.WINDOW_SHOWN, 'correct status is returned on show')
    t.equal(mainWindow.getStatus(), ApplicationWindow.constants.WINDOW_SHOWN, 'window options are updated on show')
  })

  // TODO(@chicoxyzzy): should navigation of main window throw? should navigation of current window throw? should we even allow navigation?
  test('window.navigate', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    counter++
    const { index, status } = await newWindow.navigate('frontend/index_no_js2.html')
    t.equal(index, newWindow.index, 'correct index is returned')
    t.equal(status, ApplicationWindow.constants.WINDOW_SHOWN, 'correct status is returned')
    newWindow.close()
  })

  test('window.setBackgroundColor', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    counter++
    const { index } = await newWindow.setBackgroundColor({ red: 0, green: 0, blue: 0, alpha: 0 })
    t.equal(index, newWindow.index, 'correct index is returned')
    newWindow.close()
  })

  test('window.setContextMenu', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    t.equal(typeof mainWindow.setContextMenu, 'function', 'setContextMenu is a function')
    // CI wants a user action after the context menu is opened so this stops the CI run
    // const result = await mainWindow.setContextMenu({ Foo: '', Bar: '' })
    // t.deepEqual(result, { data: null }, 'setContextMenu succeeds')
  })

  test('application.setSystemMenuItemEnabled', async (t) => {
    const result = await application.setSystemMenuItemEnabled({ indexMain: 0, indexSub: 0, enabled: true })
    t.equal(result.err, null, 'setSystemMenuItemEnabled succeeds')
  })

  test('application.setSystemMenu', async (t) => {
    const result = await application.setSystemMenu({
      index: 0, value: `
      App:
        Foo: f;
        Edit:
        Cut: x
        Copy: c
        Paste: v
        Delete: _
        Select All: a;
        Other:
        Apple: _
        Another Test: T
        !Im Disabled: I
        Some Thing: S + Meta
        ---
        Bazz: s + Meta, Control, Alt;
    `
    })
    t.equal(result.err, null, 'setSystemMenuItemVisible succeeds')
  })

  test('window.showInspector', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    t.equal(typeof mainWindow.showInspector, 'function', 'showInspector is a function')
    // const result = await mainWindow.showInspector()
    // t.equal(result, true, 'returns true')
  })

  test('window.showOpenFilePicker', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    t.ok(mainWindow.showOpenFilePicker())
  })

  test('window.showSaveFilePicker', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    t.ok(mainWindow.showSaveFilePicker())
  })

  test('window.showDirectoryFilePicker', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    t.ok(mainWindow.showDirectoryFilePicker())
  })

  test('window.send wrong window', async (t) => {
    const mainWindow = await application.getCurrentWindow()
    // passing Symbol() will throw on encodeURIComponent`
    const indices = [-1, 3.14, NaN, Infinity, -Infinity, 'IDDQD', 1n, {}, [], () => {}, true, false]
    for (const index of indices) {
      let err
      try {
        await mainWindow.send({ index, event: 'Knights who say Ni!' })
      } catch (e) {
        // they are now now Knights who say Ekke Ekke Ekke Ekke Ptang Zoo Boing!
        err = e
      }
      let printValue
      switch (typeof index) {
        case 'bigint':
          printValue = `${index}n`
          break
        case 'function':
          printValue = '() => {}'
          break
        default:
          printValue = JSON.stringify(index)
      }
      t.ok(err instanceof Error, `throws error when type ${printValue} is being passed`)
      t.equal(err?.message, 'window should be an integer', `error message is correct for ${printValue}`)
    }
  })

  test('window.send', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_send_event.html' })
    const currentWindow = await application.getCurrentWindow()
    t.equal(newWindow.index, counter, 'correct index is returned')
    // wait for window to load
    const e = await new Promise(resolve => window.addEventListener('secondary window loaded', resolve, { once: true }))
    t.ok(e instanceof Event, 'secondary window loaded event is fired')
    const value = { firstname: 'Rick', secondname: 'Sanchez' }
    currentWindow.send({ event: 'character', value, window: counter })
    const result = await new Promise(resolve => window.addEventListener('message from secondary window', resolve, { once: true }))
    t.ok(result instanceof Event, 'secondary window response event is fired')
    t.deepEqual(result.detail, value, 'send succeeds')
    newWindow.close()
    counter++
  })

  test('window.send from another window', async (t) => {
    const newWindow = await application.createWindow({ index: counter, path: 'frontend/index_no_js.html' })
    const currentWindow = await application.getCurrentWindow()
    t.equal(newWindow.index, counter, 'correct index is returned')
    t.notEqual(currentWindow.index, newWindow.index, 'window indexes are different')
    const value = { firstname: 'Morty', secondname: 'Sanchez' }
    let err
    try {
      await newWindow.send({ event: 'character', value, window: currentWindow.index })
    } catch (e) {
      err = e
    }
    t.ok(err instanceof Error, 'send from a new window throws')
    t.equal(err?.message, 'window.send can only be used from the current window', 'send from a non-current window throws')
    newWindow.close()
    counter++
  })

  test('window.send to both window and backend', async (t) => {
    const currentWindow = await application.getCurrentWindow()
    const value = { firstname: 'Morty', secondname: 'Sanchez' }
    let err
    try {
      await currentWindow.send({ event: 'character', value, window: 999, backend: true })
    } catch (e) {
      err = e
    }
    t.equal(err?.message, 'backend option cannot be used together with window option', 'send to both window and backend throws')
    counter++
  })

  test('window.send with empty event', async (t) => {
    const currentWindow = await application.getCurrentWindow()
    const value = { firstname: 'Morty', secondname: 'Sanchez' }
    let err
    try {
      await currentWindow.send({ window: 0, value })
    } catch (e) {
      err = e
    }
    t.equal(err?.message, 'event should be a non-empty string', 'send with empty event throws')
    counter++
  })

  test('window.send to backend with empty event', async (t) => {
    const currentWindow = await application.getCurrentWindow()
    const value = { firstname: 'Morty', secondname: 'Sanchez' }
    let err
    try {
      await currentWindow.send({ backend: true, value })
    } catch (e) {
      err = e
    }
    t.equal(err?.message, 'event should be a non-empty string', 'send with empty event throws')
    counter++
  })

  //
  // Backend tests
  //
  test('backend.open()', async (t) => {
    const openResult = await application.backend.open()
    t.deepEqual(Object.keys(openResult.data).sort(), ['argv', 'cmd', 'path'], 'returns a result with the correct keys')
  })

  test('backend sends to globalThis.window', async (t) => {
    const successOpenPromise = Promise.race([
      new Promise(resolve => window.addEventListener('process-error', ({ detail }) => {
        if (detail) t.fail(`process-error event emitted: ${detail}`)
        resolve(false)
      }, { once: true })),
      new Promise(resolve => setTimeout(() => resolve(true), 0))
    ])
    const backendReadyPromise = new Promise(resolve => window.addEventListener('backend:ready', () => resolve(true), { once: true }))
    const backenSendDataPromise = new Promise(resolve => window.addEventListener('character', ({ detail }) => resolve(detail), { once: true }))
    const [successOpen, backendReady, backendSendData] = await Promise.all([successOpenPromise, backendReadyPromise, backenSendDataPromise])
    t.ok(successOpen, 'does not emit a process-error event')
    t.ok(backendReady, 'can send events to window 0')
    t.deepEqual(backendSendData, { firstname: 'Morty', secondname: 'Smith' }, 'can send events with data')
  })

  test('backend.open() again', async (t) => {
    const openResult = await application.backend.open()
    t.deepEqual(Object.keys(openResult.data).sort(), ['argv', 'cmd', 'path'], 'returns a result with the correct keys')
    const doesNotRestart = await Promise.race([
      new Promise(resolve => window.addEventListener('backend:ready', () => resolve(false), { once: true })),
      new Promise(resolve => setTimeout(() => resolve(true), 512))
    ])
    t.ok(doesNotRestart, 'does not emit a backend:ready event')
  })

  test('backend.open({ force: true })', async (t) => {
    const openResult = await application.backend.open({ force: true })
    t.deepEqual(Object.keys(openResult.data).sort(), ['argv', 'cmd', 'path'], 'returns a result with the correct keys')
    const doesRestart = await Promise.race([
      new Promise(resolve => window.addEventListener('backend:ready', () => resolve(true), { once: true })),
      new Promise(resolve => setTimeout(() => resolve(false), 512))
    ])
    t.ok(doesRestart, 'emits a backend:ready event')
  })

  test('window.send to backend and back to current window', async (t) => {
    const currentWindow = await application.getCurrentWindow()
    const value = { firstname: 'Rick', secondname: 'Sanchez' }
    await currentWindow.send({ event: '20 minutes adventure', value, backend: true })
    const result = await new Promise(resolve => currentWindow.on('20 minutes adventure', ({ detail }) => resolve(detail), { once: true }))
    t.deepEqual(result, value, 'send to backend and back succeeds')
  })

  test('backend.close()', async (t) => {
    const closeResult = await application.backend.close()
    t.ok(closeResult.err == null, 'returns correct result')
    const doesNotRestart = await Promise.race([
      new Promise(resolve => window.addEventListener('backend:ready', () => resolve(false), { once: true })),
      new Promise(resolve => setTimeout(() => resolve(true), 512))
    ])
    t.ok(doesNotRestart, 'does not emit a backend:ready event')
  })
}
