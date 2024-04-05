import vm, { getTransferables } from '../vm.js'
import { Writable, Readable } from '../stream.js'
import process, { env } from '../process.js'

export const SHARE_ENV = Symbol.for('socket.runtime.worker_threads.SHARE_ENV')
export const isMainThread = Boolean(
  globalThis.window &&
  globalThis.top === globalThis.window &&
  globalThis.window === globalThis
)

export const state = {
  isMainThread,

  parentPort: isMainThread
    ? null
    : Object.create(MessagePort.prototype),

  mainPort: isMainThread
    ? Object.create(MessagePort.prototype)
    : null,

  workerData: null,
  url: null,
  env: {},
  id: 0
}

if (!isMainThread) {
  process.exit = (code) => {
    globalThis.postMessage({
      worker_threads: { process: { exit: { code } } }
    })
  }

  globalThis.addEventListener('message', onMainThreadMessage)
  globalThis.addEventListener('error', (event) => {
    propagateWorkerError(
      event.error ??
      new Error(
        event.reason?.message ??
        event.reason ??
        'An unknown error occurred'
      )
    )
  })

  globalThis.addEventListener('unhandledrejection', (event) => {
    propagateWorkerError(
      event.error ??
      new Error(
        event.reason?.message ??
        event.reason ??
        'An unknown error occurred'
      )
    )
  })
}

function propagateWorkerError (err) {
  globalThis.postMessage({
    worker_threads: {
      error: {
        name: err.name,
        type: err.constructor.name,
        code: err.code ?? undefined,
        stack: err.stack,
        message: err.message
      }
    }
  })
}

async function onMainThreadMessage (event) {
  const request = event.data?.worker_threads ?? {}

  if (request.workerData) {
    state.workerData = request.workerData
  }

  if (request.init && request.init.id && request.init.url) {
    state.id = request.init.id
    state.url = request.init.url

    if (
      request.init.process?.env &&
      typeof request.init.process.env === 'object'
    ) {
      for (const key in request.init.process.env) {
        process.env[key] = request.init.process.env[key]
      }
    } else if (request.init.process?.env === SHARE_ENV.toString()) {
      env.addEventListener('set', (event) => {
        globalThis.postMessage({
          worker_threads: {
            process: {
              env: {
                type: event.type,
                key: event.key,
                value: event.value
              }
            }
          }
        })
      })

      env.addEventListener('delete', (event) => {
        globalThis.postMessage({
          worker_threads: {
            process: {
              env: {
                type: event.type,
                key: event.key
              }
            }
          }
        })
      })
    }

    if (request.init.process?.stdin === true) {
      process.stdin = new Readable()
    }

    if (request.init.process?.stdout === true) {
      process.stdout = new Writable({
        write (data, cb) {
          const transfer = getTransferables(data)
          globalThis.postMessage({
            worker_threads: {
              process: {
                stdout: { data }
              }
            }
          }, { transfer })

          cb(null)
        }
      })
    }

    if (request.init.process?.stderr === true) {
      process.stderr = new Writable({
        write (data, cb) {
          const transfer = getTransferables(data)
          globalThis.postMessage({
            worker_threads: {
              process: {
                stderr: { data }
              }
            }
          }, { transfer })

          cb(null)
        }
      })
    }

    if (request.init.eval === true) {
      state.url = ''
      await vm.runInThisContext(request.init.url).catch(propagateWorkerError)
    } else {
      await import(state.url).catch(propagateWorkerError)
    }

    globalThis.postMessage({
      worker_threads: { online: { id: state.id } }
    })
  }

  if (request.env && typeof request.env === 'object') {
    for (const key in request.env) {
      state.env[key] = request.env
    }
  }

  if (/set|delete/.test(request.process?.env?.type ?? '')) {
    if (request.process.env.type === 'set') {
      Reflect.set(env, request.process.env.key, request.process.env.value)
    } else if (request.process.env.type === 'delete') {
      Reflect.deleteProperty(env, request.process.env.key)
    }
  }

  if (request.process?.stdin?.data && process.stdin) {
    process.stdin.push(request.process.stdin.data)
  }

  if (event.data?.worker_threads) {
    event.stopImmediatePropagation()
    return false
  }
}

if (state.parentPort) {
  let onmessageerror = null
  let onmessage = null
  Object.defineProperties(state.parentPort, {
    postMessage: {
      configurable: false,
      enumerable: false,
      value: globalThis.top
        ? globalThis.top.postMessage.bind(globalThis.top)
        : globalThis.postMessage.bind(globalThis)
    },

    close: {
      configurable: false,
      enumerable: false,
      value () {}
    },

    start: {
      configurable: false,
      enumerable: false,
      value () {}
    },

    onmessage: {
      enumerable: true,
      get: () => onmessage,
      set: (value) => {
        if (typeof onmessage === 'function') {
          globalThis.removeEventListener('message', onmessage)
        }

        onmessage = null

        if (typeof value === 'function') {
          onmessage = value
          globalThis.addEventListener('message', onmessage)
        }
      }
    },

    onmessageerror: {
      enumerable: true,
      get: () => onmessageerror,
      set: (value) => {
        if (typeof onmessageerror === 'function') {
          globalThis.removeEventListener('messageerror', onmessageerror)
        }

        onmessageerror = null

        if (typeof value === 'function') {
          onmessageerror = value
          globalThis.addEventListener('messageerror', onmessageerror)
        }
      }
    }
  })
} else if (state.mainPort) {
  let onmessageerror = null
  let onmessage = null
  Object.defineProperties(state.mainPort, {
    addEventListener: {
      configurable: false,
      enumerable: false,
      value (...args) {
        return globalThis.addEventListener(...args)
      }
    },

    removeEventListener: {
      configurable: false,
      enumerable: false,
      value (...args) {
        return globalThis.removeEventListener(...args)
      }
    },

    dispatchEvent: {
      configurable: false,
      enumerable: false,
      value (...args) {
        return globalThis.dispatchEvent(...args)
      }
    },

    onmessage: {
      enumerable: true,
      get: () => onmessage,
      set: (value) => {
        if (typeof onmessage === 'function') {
          globalThis.removeEventListener('message', onmessage)
        }

        onmessage = null

        if (typeof value === 'function') {
          onmessage = value
          globalThis.addEventListener('message', onmessage)
        }
      }
    },

    onmessageerror: {
      enumerable: true,
      get: () => onmessageerror,
      set: (value) => {
        if (typeof onmessageerror === 'function') {
          globalThis.removeEventListener('messageerror', onmessageerror)
        }

        onmessageerror = null

        if (typeof value === 'function') {
          onmessageerror = value
          globalThis.addEventListener('messageerror', onmessageerror)
        }
      }
    }
  })
}

export default { state }
