import { GLOBAL_TEST_RUNNER } from './index.js'
import console from '../console.js'
import process from '../process.js'
import ipc from '../ipc.js'

import '../application.js'

export default null

if (process.env.SOCKET_DEBUG_IPC) {
  ipc.debug.enabled = true
  ipc.debug.log = (...args) => console.log(...args)
}

if (typeof globalThis?.addEventListener === 'function') {
  globalThis.addEventListener('error', onerror)
  globalThis.addEventListener('messageerror', onerror)
  globalThis.addEventListener('unhandledrejection', onerror)
}

let finishing = false
GLOBAL_TEST_RUNNER.onFinish(({ fail }) => {
  if (!finishing) {
    finishing = true
    if (!process.env.DEBUG) {
      setTimeout(() => {
        process.exit(fail > 0 ? 1 : 0)
      }, 1024) // give app time to print TAP output
    }
  }
})

function onerror (e) {
  const err = e.error || e.stack || e.reason || e.message || e
  if (err.ignore) return
  console.error(err)
  if (!finishing && !process.env.DEBUG) {
    process.nextTick(() => {
      process.exit(1)
    })
  }
}
