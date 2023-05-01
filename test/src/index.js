/* eslint-disable import/first */
globalThis.console.assert(
  globalThis.__RUNTIME_INIT_NOW__,
  'socket:internal/init.js was not imported in preload. ' +
  'This could lead to undefined behavior.'
)

import 'socket:test/context' // this should be first

import './webview.js'
import './diagnostics.js'
import './ipc.js'
import './os.js'
import './process.js'
import './path.js'
import './dgram.js'
import './dns.js'
import './crypto.js'
import './util.js'
import './fs.js'
import './window.js'
import './application.js'
import './copy-map.js'
import './microtask.js'
import './commonjs.js'
