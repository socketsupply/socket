import { GLOBAL_TEST_RUNNER } from '@socketsupply/tapzero'
import console from '../../console.js'
import process from '../../process.js'
import '../../runtime.js'

// uncomment below to get IPC debug output in stdout
// import ipc from '../../ipc.js'
// ipc.debug.enabled = true
// ipc.debug.log = (...args) => console.log(...args)

if (typeof globalThis?.addEventListener === 'function') {
  globalThis.addEventListener('error', onerror)
  globalThis.addEventListener('unhandledrejection', onerror)
}

GLOBAL_TEST_RUNNER.onFinish(() => {
  setTimeout(() => {
    process.exit(0)
  }, 10)
})

function onerror (err) {
  console.error(err.stack || err.reason || err.message || err)
  process.exit(1)
}
