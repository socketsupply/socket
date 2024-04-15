/* global Blob, DataTransfer, DragEvent, FileList, MessageEvent, reportError */
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

import './primitives.js'
import ipc from '../ipc.js'
ipc.sendSync('platform.event', 'beforeruntimeinit')

import { CustomEvent, ErrorEvent } from '../events.js'
import { IllegalConstructor } from '../util.js'
import * as asyncHooks from './async/hooks.js'
import { Deferred } from '../async.js'
import { rand64 } from '../crypto.js'
import location from '../location.js'
import { URL } from '../url.js'
import mime from '../mime.js'
import path from '../path.js'
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

      originalQueueMicrotask(task)

      function task () {
        try {
          return asyncHooks.wrap(callback, 'Microtask').call(globalThis)
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
  globalThis.addEventListener('platformdrop', async (event) => {
    const handles = []
    let target = globalThis
    if (
      typeof event.detail?.x === 'number' && typeof event.detail?.y === 'number'
    ) {
      target = (
        globalThis.document.elementFromPoint(event.detail.x, event.detail.y) ??
        globalThis
      )
    }

    if (Array.isArray(event.detail?.files)) {
      for (const file of event.detail.files) {
        if (typeof file === 'string') {
          try {
            const stats = await fs.stat(file)
            if (stats.isDirectory()) {
              handles.push(await createFileSystemDirectoryHandle(file, { writable: false }))
            } else {
              handles.push(await createFileSystemFileHandle(file, { writable: false }))
            }
          } catch (err) {
            try {
              // try to read from navigator
              const response = await fetch(file)
              if (response.ok) {
                const lastModified = Date.now()
                const buffer = new Uint8Array(await response.arrayBuffer())
                const types = await mime.lookup(path.extname(file).slice(1))
                const type = types[0]?.mime ?? ''

                handles.push(await createFileSystemFileHandle(
                  new File(buffer, { lastModified, type }),
                  { writable: false }
                ))
              } else {
                console.warn('platformdrop: ', err)
              }
            } catch (err) {
              console.warn('platformdrop: ', err)
            }
          }
        }
      }
    }

    const dataTransfer = new DataTransfer()
    const files = []

    for (const handle of handles) {
      if (typeof handle.getFile === 'function') {
        const file = handle.getFile()
        const buffer = new Uint8Array(await file.arrayBuffer())
        files.push(new File(buffer, file.name, {
          lastModified: file.lastModified,
          type: file.type
        }))
      }
    }

    const fileList = Object.create(FileList.prototype, {
      length: {
        configurable: false,
        enumerable: false,
        get: () => files.length
      },

      item: {
        configurable: false,
        enumerable: false,
        value: (index) => files[index] ?? null
      },

      [Symbol.iterator]: {
        configurable: false,
        enumerable: false,
        get: () => files[Symbol.iterator]
      }
    })

    for (let i = 0; i < handles.length; ++i) {
      const file = files[i]
      if (file) {
        dataTransfer.items.add(file)
      } else {
        dataTransfer.items.add(handles[i].name, 'text/directory')
      }
    }

    Object.defineProperties(dataTransfer, {
      files: {
        configurable: false,
        enumerable: false,
        value: fileList
      }
    })

    const dropEvent = new DragEvent('drop', { dataTransfer })
    Object.defineProperty(dropEvent, 'detail', {
      value: {
        handles
      }
    })

    let index = 0
    for (const item of dropEvent.dataTransfer.items) {
      const handle = handles[index++]
      Object.defineProperties(item, {
        getAsFileSystemHandle: {
          configurable: false,
          enumerable: false,
          value: async () => handle
        }
      })
    }

    target.dispatchEvent(dropEvent)

    if (handles.length) {
      globalThis.dispatchEvent(new CustomEvent('dropfiles', {
        detail: {
          handles
        }
      }))
    }
  })
}

class RuntimeWorker extends GlobalWorker {
  /**
   * Internal worker pool
   * @ignore
   */
  static pool = globalThis.top?.Worker?.pool ?? new Map()

  /**
   * Handles `Symbol.species`
   * @ignore
   */
  static get [Symbol.species] () {
    return GlobalWorker
  }

  #id = null
  #objectURL = null
  #onglobaldata = null

  /**
   * `RuntimeWorker` class worker.
   * @ignore
   * @param {string|URL} filename
   * @param {object=} [options]
   */
  constructor (filename, options, ...args) {
    options = { ...options }

    const workerType = options[Symbol.for('socket.runtime.internal.worker.type')] ?? 'worker'
    const url = encodeURIComponent(new URL(filename, globalThis.location.href).toString())
    const id = String(rand64())

    const preload = `
    Object.defineProperty(globalThis, '__args', {
      configurable: false,
      enumerable: false,
      value: ${JSON.stringify(globalThis.__args)}
    })

    globalThis.__args.client.id = '${id}'
    globalThis.__args.client.type = 'worker'
    globalThis.__args.client.frameType = 'none'

    Object.defineProperty(globalThis, 'isWorkerScope', {
      configurable: false,
      enumerable: false,
      writable: false,
      value: true
    })

    Object.defineProperty(globalThis, 'isSocketRuntime', {
      configurable: false,
      enumerable: false,
      writable: false,
      value: true
    })

    Object.defineProperty(globalThis, 'RUNTIME_WORKER_ID', {
      configurable: false,
      enumerable: false,
      writable: false,
      value: '${id}'
    })

    Object.defineProperty(globalThis, 'RUNTIME_WORKER_TYPE', {
      configurable: false,
      enumerable: false,
      writable: false,
      value: '${workerType}'
    })

    Object.defineProperty(globalThis, 'RUNTIME_WORKER_LOCATION', {
      configurable: false,
      enumerable: false,
      writable: true,
      value: decodeURIComponent('${url}')
    })

    Object.defineProperty(globalThis, 'RUNTIME_WORKER_MESSAGE_EVENT_BACKLOG', {
      configurable: false,
      enumerable: false,
      value: []
    })

    globalThis.addEventListener('message', onInitialWorkerMessages)

    function onInitialWorkerMessages (event) {
      RUNTIME_WORKER_MESSAGE_EVENT_BACKLOG.push(event)
    }

    try {
      await import('${location.origin}/socket/internal/init.js')
      const hooks = await import('${location.origin}/socket/hooks.js')

      hooks.onReady(() => {
        globalThis.removeEventListener('message', onInitialWorkerMessages)
      })

      await import('${location.origin}/socket/internal/worker.js?source=${url}')
    } catch (err) {
      globalThis.reportError(err)
    }
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
    this.#objectURL = objectURL

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

    const postMessage = this.postMessage.bind(this)
    const addEventListener = this.addEventListener.bind(this)
    const removeEventListener = this.removeEventListener.bind(this)

    const postMessageQueue = []
    let isReady = false

    this.postMessage = (...args) => {
      if (!isReady) {
        postMessageQueue.push(args)
      } else {
        return postMessage(...args)
      }
    }

    this.addEventListener = (eventName, ...args) => {
      if (eventName === 'message') {
        return eventTarget.addEventListener(eventName, ...args)
      }

      return addEventListener(eventName, ...args)
    }

    this.removeEventListener = (eventName, ...args) => {
      if (eventName === 'message') {
        return eventTarget.removeEventListener(eventName, ...args)
      }

      return removeEventListener(eventName, ...args)
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

    queueMicrotask(() => {
      addEventListener('message', (event) => {
        const { data } = event
        if (data?.__runtime_worker_init === true) {
          isReady = true

          for (const args of postMessageQueue) {
            postMessage(...args)
          }

          postMessageQueue.splice(0, postMessageQueue.length)
        } else if (data?.__runtime_worker_ipc_request) {
          const request = data.__runtime_worker_ipc_request
          if (
            typeof request?.message === 'string' &&
            request.message.startsWith('ipc://')
          ) {
            queueMicrotask(async () => {
              try {
                // eslint-disable-next-line no-use-before-define
                const transfer = []
                const message = ipc.Message.from(request.message, request.bytes)
                const options = { bytes: message.bytes }
                const promise = ipc.send(message.name, message.rawParams, options)

                if (message.get('resolve') === false) {
                  return
                }

                const result = await promise

                ipc.findMessageTransfers(transfer, result)

                this.postMessage({
                  __runtime_worker_ipc_result: {
                    message: message.toJSON(),
                    result: result.toJSON()
                  }
                }, { transfer })
              } catch (err) {
                globalThis.reportError(err)
              }
            })
          }
        } else {
          return eventTarget.dispatchEvent(new MessageEvent(event.type, event))
        }
      })
    })
  }

  get id () {
    return this.#id
  }

  get objectURL () {
    return this.#objectURL
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
          url = new URL(url, globalThis.location.origin).toString()
        } catch {}
      }
    }

    const value = open.call(this, method, url, isAsyncRequest !== false, ...args)

    if (globalThis.__args?.client) {
      this.setRequestHeader('Runtime-Client-ID', globalThis.__args.client.id)
    }

    if (typeof globalThis.RUNTIME_WORKER_LOCATION === 'string') {
      this.setRequestHeader('Runtime-Worker-Location', globalThis.RUNTIME_WORKER_LOCATION)
    }

    if (globalThis.top && globalThis.top !== globalThis) {
      this.setRequestHeader('Runtime-Frame-Type', 'nested')
    } else if (!globalThis.window && globalThis.self === globalThis) {
      this.setRequestHeader('Runtime-Frame-Type', 'worker')
      if (globalThis.clients && globalThis.FetchEvent) {
        this.setRequestHeader('Runtime-Worker-Type', 'serviceworker')
      } else {
        this.setRequestHeader('Runtime-Worker-Type', 'worker')
      }
    } else {
      this.setRequestHeader('Runtime-Frame-Type', 'top-level')
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
import '../console.js'

ipc.sendSync('platform.event', {
  value: 'load',
  'location.href': globalThis.location.href
})

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
      if (RuntimeWorker.pool.has(options.workerId)) {
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
    }

    const promise = new Deferred()
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

hooks.onLoad(async () => {
  const registeredServiceWorkers = new Set()
  const serviceWorkerScripts = config['webview_service-workers']
  const pending = []

  if (
    globalThis.window &&
    !globalThis.__RUNTIME_SERVICE_WORKER_CONTEXT__ &&
    globalThis.location.pathname !== '/socket/service-worker/index.html' &&
    String(config.permissions_allow_service_worker) !== 'false' &&
    String(config.webview_auto_register_service_workers) !== 'false'
  ) {
    const pendingServiceRegistrations = []

    if (typeof config['webview_service-workers'] === 'string') {
      for (const scriptURL of serviceWorkerScripts.trim().split(' ')) {
        pendingServiceRegistrations.push({
          scriptURL: scriptURL.trim(),
          options: {}
        })
      }
    }

    for (const key in config) {
      if (key.startsWith('webview_service-workers_')) {
        const scope = key.replace('webview_service-workers_', '')
        const scriptURL = config[key].trim()
        pendingServiceRegistrations.push({
          scriptURL,
          options: { scope }
        })
      }
    }

    for (const registration of pendingServiceRegistrations) {
      const { options } = registration
      let { scriptURL } = registration

      if (!scriptURL.startsWith('/') && scriptURL.startsWith('.')) {
        if (!URL.canParse(scriptURL, globalThis.location.href)) {
          scriptURL = `./${scriptURL}`
        }
      }

      const url = new URL(scriptURL, globalThis.location.origin)
      const scope = options.scope ?? new URL('.', url).pathname

      if (!globalThis.location.pathname.startsWith(scope)) {
        continue
      }

      if (registeredServiceWorkers.has(scriptURL)) {
        continue
      }

      const promise = globalThis.navigator.serviceWorker.register(scriptURL, options)
      registeredServiceWorkers.add(scriptURL)

      pending.push(promise)

      promise
        .then((registration) => {
          if (!registration) {
            console.warn(
              'ServiceWorker failed to register in preload: %s',
              scriptURL
            )
          }
        })
        .catch((err) => {
          console.error(
            'ServiceWorker registration error occurred in preload: %s:',
            scriptURL,
            err
          )
        })
    }
  }

  await Promise.all(pending)
  if (typeof globalThis.dispatchEvent === 'function') {
    globalThis.__RUNTIME_INIT_NOW__ = performance.now()
    globalThis.dispatchEvent(new RuntimeInitEvent())
  }

  if (globalThis.document) {
    const beginRuntimePreload = globalThis.document.querySelector('meta[name=begin-runtime-preload]')
    if (beginRuntimePreload) {
      let current = beginRuntimePreload
      while (current) {
        const next = current.nextElementSibling
        current.remove()
        if (current.tagName === 'META' && current.name === 'end-runtime-preload') {
          current = null
        } else {
          current = next
        }
      }
    }
  }
})

// async preload modules
hooks.onReady(async () => {
  try {
    // precache 'fs.constants' and 'os.constants'
    await ipc.request('fs.constants', {}, { cache: true })
    await ipc.request('os.constants', {}, { cache: true })
    await import('../diagnostics.js')
    await import('../signal.js')
    await import('../fs/fds.js')
    await import('../constants.js')
    const errors = await import('../errors.js')
    // lazily install this
    const errno = await import('../errno.js')
    errors.ErrnoError.errno = errno
  } catch (err) {
    console.error(err.stack || err)
  }
})

// symbolic globals
globals.register('RuntimeXHRPostQueue', new RuntimeXHRPostQueue())
globals.register('RuntimeExecution', new asyncHooks.CoreAsyncResource('RuntimeExecution'))

// prevent further construction if this class is indirectly referenced
RuntimeXHRPostQueue.prototype.constructor = IllegalConstructor
Object.defineProperty(globalThis, '__globals', {
  enumerable: false,
  configurable: false,
  value: globals
})

ipc.send('platform.event', 'runtimeinit').catch(reportError)

export default {
  location
}
