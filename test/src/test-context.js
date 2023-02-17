import { GLOBAL_TEST_RUNNER } from 'socket:test'
import console from 'socket:console'
import process from 'socket:process'
import 'socket:application'
import ipc from 'socket:ipc'

if (process.env.SOCKET_DEBUG_IPC) {
  ipc.debug.enabled = true
  ipc.debug.log = (...args) => console.log(...args)
}

if (typeof globalThis?.addEventListener === 'function') {
  globalThis.addEventListener('error', onerror)
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

function onerror (err) {
  console.error(err.stack || err.reason || err.message || err)
  if (!finishing && !process.env.DEBUG) {
    process.nextTick(() => {
      process.exit(1)
    })
  }
}
