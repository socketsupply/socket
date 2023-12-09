/* global Blob, ArrayBuffer */
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

import './monkeypatch.js'

import { IllegalConstructor, InvertedPromise } from '../util.js'
import { CustomEvent, ErrorEvent } from '../events.js'
import { rand64 } from '../crypto.js'
import location from '../location.js'
import { URL } from '../url.js'
import fs from '../fs/promises.js'
import {
  createFileSystemDirectoryHandle,
  createFileSystemFileHandle
} from '../fs/web.js'

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

// webview only features
if ((globalThis.window) === globalThis) {
  globalThis.addEventListener('dragdropfiles', async (event) => {
    const { files } = event.detail
    const handles = []
    if (Array.isArray(files)) {
      for (const file of files) {
        if (typeof file === 'string') {
          const stats = await fs.stat(file)
          if (stats.isDirectory()) {
            handles.push(await createFileSystemDirectoryHandle(file, { writable: false }))
          } else {
            handles.push(await createFileSystemFileHandle(file, { writable: false }))
          }
        }
      }
    }

    globalThis.dispatchEvent(new CustomEvent('dropfiles', { detail: { handles } }))
  })
}

class RuntimeWorker extends GlobalWorker {
  #onglobaldata = null
  #id = rand64()

  static pool = new Map()

  static get [Symbol.species] () {
    return GlobalWorker
  }

  /**
   * `RuntimeWorker` class worker.
   * @ignore
   */
  constructor (filename, options, ...args) {
    const url = encodeURIComponent(new URL(filename, location.href || '/').toString())
    const id = rand64()

    const preload = `
    Object.defineProperty(globalThis, '__args', {
      configurable: false,
      enumerable: false,
      value: ${JSON.stringify(globalThis.__args)}
    })

    Object.defineProperty(globalThis, 'RUNTIME_WORKER_ID', {
      configurable: false,
      enumerable: false,
      value: '${id}'
    })

    Object.defineProperty(globalThis, 'RUNTIME_WORKER_LOCATION', {
      configurable: false,
      enumerable: false,
      value: '${url}'
    })

    import 'socket://${location.hostname}/socket/internal/worker.js?source=${url}'
    `.trim()

    const objectURL = URL.createObjectURL(
      new Blob([preload.trim()], { type: 'application/javascript' })
    )

    // level 1 worker `EventTarget` 'message' listener
    let onmessage = null

    // events are routed through this `EventTarget`
    const eventTarget = new EventTarget()

    super(objectURL, { ...options, type: 'module' }, ...args)

    RuntimeWorker.pool.set(id, new WeakRef(this))

    this.#id = id

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

    this.addEventListener = (eventName, ...args) => {
      if (eventName === 'message') {
        return eventTarget.addEventListener(eventName, ...args)
      }

      return this.addEventListener(eventName, ...args)
    }

    this.removeEventListener = (eventName, ...args) => {
      if (eventName === 'message') {
        return eventTarget.removeEventListener(eventName, ...args)
      }

      return this.removeEventListener(eventName, ...args)
    }

    Object.defineProperty(this, 'onmessage', {
      configurable: false,
      get: () => onmessage,
      set: (value) => {
        if (typeof onmessage === 'function') {
          eventTarget.removeEventListener('message', onmessage)
        }

        if (value === null || typeof value === 'function') {
          onmessage = value
          eventTarget.addEventListener('message', onmessage)
        }
      }
    })

    this.addEventListener('message', (event) => {
      const { data } = event
      if (data?.__runtime_worker_ipc_request) {
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
              const result = await ipc.request(message.name, message.rawParams, options)
              const transfer = []

              if (ArrayBuffer.isView(result.data) || result.data instanceof ArrayBuffer) {
                transfer.push(result.data)
              }

              this.postMessage({
                __runtime_worker_ipc_result: {
                  message: message.toJSON(),
                  result: result.toJSON()
                }
              }, transfer)
            } catch (err) {
              console.warn('RuntimeWorker:', err)
            }
          })
        }
      } else {
        return eventTarget.dispatchEvent(event)
      }
    })
  }

  get id () {
    return this.#id
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

  globalThis.XMLHttpRequest.prototype.open = function (method, url, isAsyncRequest, ...args) {
    Object.defineProperty(this, isAsync, {
      configurable: false,
      enumerable: false,
      writable: false,
      value: isAsyncRequest !== false
    })

    if (typeof url === 'string') {
      if (url.startsWith('/') || url.startsWith('.')) {
        try {
          url = new URL(url, location.origin).toString()
        } catch {}
      }
    }

    const value = open.call(this, method, url, isAsyncRequest !== false, ...args)

    if (typeof globalThis.RUNTIME_WORKER_ID === 'string') {
      this.setRequestHeader('Runtime-Worker-ID', globalThis.RUNTIME_WORKER_ID)
    }

    if (typeof globalThis.RUNTIME_WORKER_LOCATION === 'string') {
      this.setRequestHeader('Runtime-Worker-Location', globalThis.RUNTIME_WORKER_LOCATION)
    }

    return value
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

import hooks, { RuntimeInitEvent } from '../hooks.js'
import { config } from '../application.js'
import globals from './globals.js'
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
      if (event.defaultPrevented !== true) {
        // @ts-ignore
        const { error, type } = event
        globalThis.dispatchEvent?.(new ErrorEvent(type, { error }))
      }
    })
  }

  async wait () {
    if (this.pending.length < this.concurrency) return
    const offset = (this.pending.length - this.concurrency) + 1
    const pending = this.pending.slice(0, offset)
    await Promise.all(pending)
  }

  peek () {
    return this.pending[0]
  }

  timeout (request, timer) {
    let timeout = null
    const onresolve = () => {
      clearTimeout(timeout)
      const index = this.pending.indexOf(request)
      if (index > -1) {
        this.pending.splice(index, 1)
      }
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
  async dispatch (id, seq, params, headers, options = null) {
    if (options?.workerId) {
      const worker = RuntimeWorker.pool.get(options.workerId)?.deref?.()
      if (worker) {
        worker.postMessage({
          __runtime_worker_event: {
            type: 'runtime-xhr-post-queue',
            detail: {
              id, seq, params, headers
            }
          }
        })
        return
      }
    }

    const promise = new InvertedPromise()
    await this.push(promise, 8)

    if (typeof params !== 'object') {
      params = {}
    }

    const result = await ipc.request('post', { id }, {
      responseType: 'arraybuffer'
    })

    promise.resolve()

    if (result.err) {
      this.dispatchEvent(new ErrorEvent('error', { error: result.err }))
    } else {
      const { data } = result
      const detail = { headers, params, data, id }
      globalThis.dispatchEvent(new CustomEvent('data', { detail }))
    }
  }
}

hooks.onLoad(() => {
  if (typeof globalThis.dispatchEvent === 'function') {
    globalThis.__RUNTIME_INIT_NOW__ = performance.now()
    globalThis.dispatchEvent(new RuntimeInitEvent())
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

export default null
