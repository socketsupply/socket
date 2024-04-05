import globals from '../internal/globals.js'
import util from '../util.js'

export function debug (...args) {
  const state = globals.get('ServiceWorker.state')

  if (process.env.SOCKET_RUTIME_SERVICE_WORKER_DEBUG) {
    console.debug(...args)
  }

  if (args[0] instanceof Error) {
    globalThis.postMessage({
      __service_worker_debug: [
        `[${state.serviceWorker.scriptURL}]: ${util.format(...args)}`
      ]
    })

    if (typeof state?.reportError === 'function') {
      state.reportError(args[0])
    } else if (typeof globalThis.reportError === 'function') {
      globalThis.reportError(args[0])
    }
  } else {
    globalThis.postMessage({ __service_worker_debug: [util.format(...args)] })
  }
}

export default debug
