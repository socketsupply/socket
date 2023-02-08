import { GLOBAL_TEST_RUNNER } from 'socket:test'
import console from 'socket:console'
import process from 'socket:process'
import 'socket:runtime'

// uncomment below to get IPC debug output in stdout
// import ipc from 'socket:ipc'
// ipc.debug.enabled = true
// ipc.debug.log = (...args) => console.log(...args)

if (typeof globalThis?.addEventListener === 'function') {
  globalThis.addEventListener('error', onerror)
  globalThis.addEventListener('unhandledrejection', onerror)
}

GLOBAL_TEST_RUNNER.onFinish(({ fail }) => {
  setTimeout(() => {
    process.exit(fail > 0 ? 1 : 0)
  }, 1024) // give app time to print TAP output
})

function onerror (err) {
  console.error(err.stack || err.reason || err.message || err)
  process.exit(1)
}
