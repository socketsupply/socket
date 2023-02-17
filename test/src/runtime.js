import { test } from 'socket:test'
import runtime from 'socket:runtime'
import process from 'socket:process'

// TODO(@jwerle): FIXME
if (process.platform !== 'win32') {
  // Desktop-only runtime functions
  if (!['android', 'ios'].includes(process.platform)) {
    test('send', async (t) => {
      await runtime.show({
        window: 2,
        url: 'index_send_event.html',
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
<<<<<<< HEAD

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

    test('config', async (t) => {
      const rawConfig = await readFile('socket.ini', 'utf8')
      const config = []
      const lines = rawConfig.split('\n')
      let prefix = ''

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

      config.forEach(([key, value]) => {
        switch (key) {
          case 'build_headless':
            t.equal(runtime.config[key].toString(), value, `runtime.config.${key} is correct`)
            break
          case 'build_name':
            t.ok(runtime.config[key].startsWith(value), `runtime.config.${key} is correct`)
            break
          default:
            t.equal(runtime.config[key], value, `runtime.config.${key} is correct`)
        }

        t.throws(
          () => { runtime.config[key] = 0 },
          // eslint-disable-next-line prefer-regex-literals
          RegExp('Attempted to assign to readonly property.'),
          `runtime.config.${key} is read-only`
        )
      })
    })
=======
>>>>>>> b656f079 (add system menus)
  }

<<<<<<< HEAD
  // Desktop + mobile runtime functions
  test('debug', (t) => {
    t.equal(runtime.debug, window.__args.debug, 'debug is correct')
    t.throws(() => { runtime.debug = 1 }, 'debug is immutable')
  })

  test('version', (t) => {
    t.equal(runtime.version.short, primordials.version.short, 'short version is correct')
    t.equal(runtime.version.hash, primordials.version.hash, 'version hash is correct')
    t.equal(runtime.version.full, primordials.version.full, 'full version is correct')
  })

  test('currentWindow', (t) => {
    t.equal(runtime.currentWindow, window.__args.index, 'runtime.currentWindow equals window.__args.index')
    t.equal(runtime.currentWindow, 0, 'runtime.currentWindow equals 0')
    t.throws(() => { runtime.currentWindow = 1 }, 'runtime.currentWindow is immutable')
  })

=======
>>>>>>> 565a0315 (rename messages)
  test('openExternal', async (t) => {
    t.equal(typeof runtime.openExternal, 'function', 'openExternal is a function')
    // can't test results without browser
    // t.equal(await runtime.openExternal('https://sockets.sh'), null, 'succesfully completes')
  })

  // We don't need to test runtime.exit. It works if the app exits after the tests.
}
