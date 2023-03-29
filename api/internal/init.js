/* global CustomEvent, ErrorEvent, Blob */
/* eslint-disable import/first */
// mark when runtime did init
console.assert(
  !globalThis.__RUNTIME_INIT_NOW__,
  'socket:internal/init.js was imported twice. ' +
  'This could lead to undefined behavior.'
)
console.assert(
  !globalThis.process?.versions?.node,
  'socket:internal/init.js was imported in Node.js. ' +
  'This could lead to undefined behavior.'
)

globalThis.__RUNTIME_INIT_NOW__ = performance.now()

const GlobalWorker = globalThis.Worker || class Worker extends EventTarget {}

// only patch a webview or worker context
if ((globalThis.window || globalThis.self) === globalThis) {
  if (typeof globalThis.queueMicrotask === 'function') {
    const originalQueueMicrotask = globalThis.queueMicrotask
    const promise = Promise.resolve()

    globalThis.queueMicrotask = queueMicrotask
    globalThis.addEventListener('queuemicrotaskerror', (e) => {
      throw e.error
    })

    function queueMicrotask (...args) {
      if (args.length === 0) {
        throw new TypeError('Not enough arguments')
      }

      const [callback] = args

      if (typeof callback !== 'function') {
        throw new TypeError(
          'Argument 1 (\'callback\') to globalThis.queueMicrotask ' +
          'must be a function'
        )
      }

      return originalQueueMicrotask(task)

      function task () {
        try {
          return callback.call(globalThis)
        } catch (error) {
          // XXX(@jwerle): `queueMicrotask()` is broken in WebKit WebViews
          // If an error is thrown, it does not bubble to the `globalThis`
          // object, but is instead silently discarded. This is misleading
          // and results in confusion for developers trying to debug something
          // they may have done wrong. Here we will rethrow the exception out
          // of microtask execution context with the best function we can
          // possibly use to report the exception to the `globalThis` object
          // as an `Unhandled Promise Rejection` error
          const event = new ErrorEvent('queuemicrotaskerror', { error })
          promise.then(() => globalThis.dispatchEvent(event))
        }
      }
    }
  }
}

class RuntimeWorker extends GlobalWorker {
  #onmessage = null
  #onglobaldata = null

  static get [Symbol.species] () { return GlobalWorker }
  constructor (filename, options, ...args) {
    const url = new URL(filename, globalThis.location?.href || '/')
    const preload = `import 'socket:internal/worker?source=${url}'`
    const blob = new Blob([preload.trim()], { type: 'application/javascript' })
    const uri = URL.createObjectURL(blob)

    super(uri, { ...options, type: 'module' }, ...args)

    this.#onglobaldata = (event) => {
      const data = new Uint8Array(event.detail.data).buffer
      this.postMessage({
        __runtime_worker_event: {
          type: event.type,
          detail: {
            ...event.detail,
            data
          }
        }
      }, [data])
    }

    globalThis.addEventListener('data', this.#onglobaldata)

    this.addEventListener('message', (event) => {
      const { data } = event
      if (data?.__runtime_worker_request_args) {
        this.postMessage({
          __runtime_worker_args: globalThis.__args
        })
      } else if (data?.__runtime_worker_ipc_request) {
        const request = data.__runtime_worker_ipc_request
        if (
          typeof request?.message === 'string' &&
          request.message.startsWith('ipc://')
        ) {
          queueMicrotask(async () => {
            try {
              // eslint-disable-next-line no-use-before-define
              const message = ipc.Message.from(request.message, request.bytes)
              const options = { bytes: message.bytes }
              // eslint-disable-next-line no-use-before-define
              const result = await ipc.send(message.name, message.rawParams, options)
              this.postMessage({
                __runtime_worker_ipc_result: {
                  message: message.toJSON(),
                  result: result.toJSON()
                }
              })
            } catch (err) {
              console.warn('RuntimeWorker:', err)
            }
          })
        }
      } else if (typeof this.onmessage === 'function') {
        return this.onmessage(event)
      }
    })
  }

  get onmessage () {
    return this.#onmessage
  }

  set onmessage (onmessage) {
    this.#onmessage = onmessage
  }

  terminate () {
    globalThis.removeEventListener('data', this.#onglobaldata)
    return super.terminate()
  }
}

// patch `globalThis.Worker`
if (globalThis.Worker === GlobalWorker) {
  globalThis.Worker = RuntimeWorker
}

// patch `globalThis.XMLHttpRequest`
if (typeof globalThis.XMLHttpRequest === 'function') {
  const { open, send } = globalThis.XMLHttpRequest.prototype
  const isAsync = Symbol('isAsync')
  let queue = null

  globalThis.XMLHttpRequest.prototype.open = function (...args) {
    const [,, async] = args
    Object.defineProperty(this, isAsync, {
      configurable: false,
      enumerable: false,
      writable: false,
      value: async !== false
    })

    return open.call(this, ...args)
  }

  globalThis.XMLHttpRequest.prototype.send = async function (...args) {
    if (!this[isAsync]) {
      return await send.call(this, ...args)
    }

    if (!queue) {
      // eslint-disable-next-line no-use-before-define
      queue = new ConcurrentQueue(
        // eslint-disable-next-line no-use-before-define
        parseInt(config.webview_xhr_concurrency)
      )
    }

    await queue.push(new Promise((resolve) => {
      this.addEventListener('error', resolve)
      this.addEventListener('readystatechange', () => {
        if (
          this.readyState === globalThis.XMLHttpRequest.DONE ||
          this.readyState === globalThis.XMLHttpRequest.UNSENT
        ) {
          resolve()
        }
      })
    }))

    return await send.call(this, ...args)
  }
}

import { IllegalConstructor, InvertedPromise } from '../util.js'
import { applyPolyfills } from '../polyfills.js'
import { config } from '../application.js'
import globals from './globals.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

import '../console.js'

const isWorkerLike = !globalThis.window && globalThis.self && globalThis.postMessage

class ConcurrentQueue extends EventTarget {
  concurrency = Infinity
  pending = []

  constructor (concurrency) {
    super()
    if (typeof concurrency === 'number' && concurrency > 0) {
      this.concurrency = concurrency
    }

    this.addEventListener('error', (event) => {
      // @ts-ignore
      const { error, type } = event
      globalThis.dispatchEvent?.(new ErrorEvent(type, { error }))
    })
  }

  async wait () {
    if (this.pending.length < this.concurrency) return
    await this.peek()
  }

  peek () {
    return this.pending[0]
  }

  timeout (request, timer) {
    let timeout = null
    const onresolve = () => {
      clearTimeout(timeout)
      queueMicrotask(() => {
        const index = this.pending.indexOf(request)
        if (index > -1) {
          this.pending.splice(index, 1)
        }
      })
    }

    timeout = setTimeout(onresolve, timer || 32)
    return onresolve
  }

  async push (request, timer) {
    await this.wait()
    this.pending.push(request)
    request.then(this.timeout(request, timer))
  }
}

class RuntimeXHRPostQueue extends ConcurrentQueue {
  async dispatch (id, seq, params, headers) {
    const promise = new InvertedPromise()
    await this.push(promise, 8)

    if (typeof params !== 'object') {
      params = {}
    }

    params = { ...params, id }
    const options = { responseType: 'arraybuffer' }
    const result = await ipc.request('post', { id }, options)

    if (result.err) {
      promise.resolve()
      this.dispatchEvent(new CustomEvent('error', { detail: result.err }))
      return
    }

    Promise.resolve().then(() => {
      const { data } = result
      const detail = { headers, params, data, id }
      promise.resolve()
      globalThis.dispatchEvent(new CustomEvent('data', { detail }))
    })
  }
}

hooks.onLoad(() => {
  if (typeof globalThis.dispatchEvent === 'function') {
    globalThis.dispatchEvent(new CustomEvent('init'))
  }
})

// async preload modules
hooks.onReady(async () => {
  try {
    if (!isWorkerLike) {
      // precache fs.constants
      await ipc.request('fs.constants', {}, { cache: true })
    }

    await import('../diagnostics.js')
    await import('../fs/fds.js')
    await import('../fs/constants.js')
  } catch (err) {
    console.error(err.stack || err)
  }
})

// symbolic globals
globals.register('RuntimeXHRPostQueue', new RuntimeXHRPostQueue())
// prevent further construction if this class is indirectly referenced
RuntimeXHRPostQueue.prototype.constructor = IllegalConstructor
export default applyPolyfills()
