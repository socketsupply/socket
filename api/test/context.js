import console from '../console.js'
import process from '../process.js'

import '../application.js'

let finishing = false
let isInitialized = false

function onerror (e) {
  const err = e.error || e.stack || e.reason || e.message || e
  if (err.ignore || err[Symbol.for('socket.test.error.ignore')]) return
  console.error(err)
  if (!finishing && !process.env.DEBUG) {
    process.nextTick(() => {
      process.exit(1)
    })
  }
}

export default function (GLOBAL_TEST_RUNNER) {
  if (isInitialized) return

  if (typeof globalThis?.addEventListener === 'function') {
    globalThis.addEventListener('error', onerror)
    globalThis.addEventListener('messageerror', onerror)
    globalThis.addEventListener('unhandledrejection', onerror)
  }

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

  isInitialized = true
}
