import runtime from '../../runtime.js'
import { readFile } from '../../fs/promises.js'
import ipc from '../../ipc.js'
import { test } from '@socketsupply/tapzero'

// Desktop-only runtime functions
if (window.__args.os !== 'android' && window.__args.os !== 'ios') {
  // Polyfills
  test('window.resizeTo', async (t) => {
    t.equal(typeof window.resizeTo, 'function', 'window.resizeTo is a function')
    t.ok(window.resizeTo(420, 200), 'succesfully completes')
    const { data: { width, height } } = await ipc.send('window.getSize')
    t.equal(width, 420, 'width is 420')
    t.equal(height, 200, 'height is 200')
  })

  test('window.document.title', async (t) => {
    window.document.title = 'idkfa'
    t.equal(window.document.title, 'idkfa', 'window.document.title is has been changed')
    t.notEqual(window.__args.title, window.document.title, 'window.__args.title is not changed')
    const { data: { title } } = await ipc.send('window.getTitle')
    t.equal(title, 'idkfa', 'window title is correct')
  })

  // Other runtime tests

  test('currentWindow', (t) => {
    t.equal(runtime.currentWindow, window.__args.index, 'runtime.currentWindow equals window.__args.index')
    t.equal(runtime.currentWindow, 0, 'runtime.currentWindow equals 0')
    t.throws(() => { runtime.currentWindow = 1 }, 'runtime.currentWindow is immutable')
  })

  test('debug', (t) => {
    t.equal(runtime.debug, window.__args.debug, 'debug is correct')
    t.throws(() => { runtime.debug = 1 }, 'debug is immutable')
  })

  test('config', async (t) => {
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
      value = value.trim().replace(/"/g, '')
      config.push([prefix.length === 0 ? key : prefix + '_' + key, value])
    }
    config.filter(([key]) => key !== 'headless' && key !== 'name').forEach(([key, value]) => {
      t.equal(runtime.config[key], value, `runtime.config.${key} is correct`)
      t.throws(
        () => { runtime.config[key] = 0 },
        // eslint-disable-next-line prefer-regex-literals
        RegExp('Attempted to assign to readonly property.'),
        `runtime.config.${key} is read-only`
      )
    })
    t.equal(runtime.config.headless, true, 'runtime.config.headless is correct')
    t.throws(
      () => { runtime.config.hedless = 0 },
      // eslint-disable-next-line prefer-regex-literals
      RegExp('Attempting to define property on object that is not extensible.'),
      'runtime.config.headless is read-only'
    )
    t.ok(runtime.config.name.startsWith(config.find(([key]) => key === 'name')[1]), 'runtime.config.name is correct')
    t.throws(
      () => { runtime.config.name = 0 },
      // eslint-disable-next-line prefer-regex-literals
      RegExp('Attempted to assign to readonly property.'),
      'runtime.config.name is read-only'
    )
  })

  test('setTitle', async (t) => {
    t.equal(typeof runtime.setTitle, 'function', 'setTitle is a function')
    const result = await runtime.setTitle('test')
    t.equal(result.err, null, 'succesfully completes')
  })

  // TODO: it crashes the app
  test('inspect', async (t) => {
    t.equal(typeof runtime.inspect, 'function', 'inspect is a function')
  })

  test('show', async (t) => {
    t.equal(typeof runtime.show, 'function', 'show is a function')
    await runtime.show({
      window: 1,
      url: 'index_second_window.html',
      title: 'Hello World',
      width: 400,
      height: 400
    })
    const { data: { status, index: i1 } } = await ipc.send('window.getStatus', { window: 1 })
    const { data: { title, index: i2 } } = await ipc.send('window.getTitle', { window: 1 })
    const { data: { height, width, index: i3 } } = await ipc.send('window.getSize', { window: 1 })
    t.equal(status, 31, 'window is shown')
    t.equal(title, 'Hello World', 'window title is correct')
    t.equal(height, 400, 'window height is correct')
    t.equal(width, 400, 'window width is correct')
    t.equal(i1, 1, 'window index is correct')
    t.equal(i2, 1, 'window index is correct')
    t.equal(i3, 1, 'window index is correct')
  })

  test('send', async (t) => {
    await runtime.show({
      window: 2,
      url: 'index_second_window.html',
      title: 'Hello World',
      width: 400,
      height: 400
    })
    // wait for window to load
    await Promise.all([
      new Promise(resolve => window.addEventListener('secondary window 1 loaded', resolve, { once: true })),
      new Promise(resolve => window.addEventListener('secondary window 2 loaded', resolve, { once: true }))
    ])
    t.equal(typeof runtime.send, 'function', 'send is a function')
    const value = { firstname: 'Rick', secondname: 'Sanchez' }
    runtime.send({ event: 'character', value })
    const [result, pong] = await Promise.all([
      new Promise(resolve => window.addEventListener('message from secondary window 1', e => resolve(e.detail))),
      new Promise(resolve => window.addEventListener('message from secondary window 2', e => resolve(e.detail)))
    ])
    t.deepEqual(result, value, 'send succeeds')
    t.deepEqual(pong, value, 'send back from window 1 succeeds')
  })

  test('getWindows', async (t) => {
    const { data: windows } = await runtime.getWindows()
    t.ok(Array.isArray(windows), 'windows is an array')
    t.ok(windows.length > 0, 'windows is not empty')
    t.ok(windows.every(w => Number.isInteger(w)), 'windows are integers')
    t.deepEqual(windows, [0, 1, 2], 'windows are correct')
  })

  test('getWindows with props', async (t) => {
    await runtime.show()
    const { data: windows1 } = await runtime.getWindows({ title: true, size: true, status: true })
    t.ok(Array.isArray(windows1), 'windows is an array')
    t.ok(windows1.length > 0, 'windows is not empty')
    t.deepEqual(windows1, [
      {
        title: 'test',
        width: 420,
        height: 200,
        status: 31,
        index: 0
      },
      {
        title: 'Secondary window',
        width: 400,
        height: 400,
        status: 31,
        index: 1
      },
      {
        title: 'Secondary window',
        width: 400,
        height: 400,
        status: 31,
        index: 2
      }
    ], 'windows are correct')
  })

  test('navigate', async (t) => {
    t.equal(typeof runtime.navigate, 'function', 'navigate is a function')
    const result = await runtime.navigate({ window: 1, url: 'index_second_window2.html' })
    t.equal(result.err, null, 'navigate succeeds')
  })

  test('hide', async (t) => {
    t.equal(typeof runtime.hide, 'function', 'hide is a function')
    await runtime.hide({ window: 1 })
    await runtime.hide({ window: 2 })
    const { data: statuses } = await runtime.getWindows({ status: true })
    t.deepEqual(statuses, [
      {
        status: 31,
        index: 0
      },
      {
        status: 21,
        index: 1
      },
      {
        status: 21,
        index: 2
      }
    ], 'statuses are correct')
  })

  test('setWindowBackgroundColor', async (t) => {
    t.equal(typeof runtime.setWindowBackgroundColor, 'function', 'setWindowBackgroundColor is a function')
    const result = await runtime.setWindowBackgroundColor({ red: 0, green: 0, blue: 0, alpha: 0 })
    // TODO: should be result.err === null?
    t.equal(result, null, 'setWindowBackgroundColor succeeds')
  })

  // FIXME: it hangs
  test('setContextMenu', async (t) => {
    t.equal(typeof runtime.setContextMenu, 'function', 'setContextMenu is a function')
    // const result = await runtime.setContextMenu({ 'Foo': '', 'Bar': '' })
    // t.equal(result, null, 'setContextMenu succeeds')
  })

  test('setSystemMenuItemEnabled', async (t) => {
    t.equal(typeof runtime.setSystemMenuItemEnabled, 'function', 'setSystemMenuItemEnabled is a function')
    const result = await runtime.setSystemMenuItemEnabled({ indexMain: 0, indexSub: 0, enabled: true })
    t.equal(result.err, null, 'setSystemMenuItemEnabled succeeds')
  })

  test('setSystemMenu', async (t) => {
    t.equal(typeof runtime.setSystemMenu, 'function', 'setSystemMenuItemVisible is a function')
    const result = await runtime.setSystemMenu({
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
}

// Common runtime functions
test('openExternal', async (t) => {
  t.equal(typeof runtime.openExternal, 'function', 'openExternal is a function')
  // can't test results without browser
  // t.equal(await runtime.openExternal('https://sockets.sh'), null, 'succesfully completes')
})

test('window.showOpenFilePicker', async (t) => {
  t.equal(typeof window.showOpenFilePicker, 'function', 'window.showOpenFilePicker is a function')
  t.ok(window.showOpenFilePicker())
})

test('window.showSaveFilePicker', (t) => {
  t.equal(typeof window.showSaveFilePicker, 'function', 'window.showSaveFilePicker is a function')
  t.ok(window.showSaveFilePicker())
})

test('window.showDirectoryFilePicker', (t) => {
  t.equal(typeof window.showDirectoryFilePicker, 'function', 'window.showDirectoryFilePicker is a function')
  t.ok(window.showDirectoryFilePicker())
})

// TODO: can we improve this test?
test('reload', (t) => {
  t.equal(typeof runtime.reload, 'function', 'reload is a function')
})

// We don't need to test runtime.exit. It works if the app exits after the tests.
