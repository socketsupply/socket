/* global reportError, EventTarget, CustomEvent, MessageEvent */
import { rand64 } from '../crypto.js'
import { Loader } from '../commonjs/loader.js'
import globals from './globals.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

export const WorkerGlobalScopePrototype = globalThis.WorkerGlobalScope?.prototype ?? {}

// determine if in actual worker scope
const isWorkerScope = Boolean(globalThis.self && !globalThis.window)

// conencted `MessagePort` instances when in `SharedWorker` mode
const ports = []

// level 1 Worker `EventTarget` 'message' listener
let onmessage = null

// level 1 Worker `EventTarget` 'connect' listener
let onconnect = null

// 'close' state for a polyfilled `SharedWorker`
let isClosed = false

// events are routed through this `EventTarget`
const workerGlobalScopeEventTarget = new EventTarget()

/**
 * The absolute `URL` of the internal worker initialization entry.
 * @ignore
 * @type {URL}
 */
// @ts-ignore
export const url = new URL(import.meta.url)

/**
 * The worker entry source.
 * @ignore
 * @type {string}
 */
export const source = globalThis.RUNTIME_WORKER_LOCATION ?? url.searchParams.get('source') ?? null

/**
 * A unique identifier for this worker made available on the global scope
 * @ignore
 * @type {string}
 */
export const RUNTIME_WORKER_ID = globalThis.RUNTIME_WORKER_ID ?? rand64().toString()

/**
 * Internally scoped event interface for a worker context.
 * @ignore
 * @type {object}
 */
export const worker = {
  postMessage: globalThis.postMessage.bind(globalThis),
  addEventListener: globalThis.addEventListener.bind(globalThis),
  removeEventListener: globalThis.removeEventListener.bind(globalThis),
  dispatchEvent: globalThis.dispatchEvent.bind(globalThis),
  close: globalThis.close.bind(globalThis)
}

/**
 * A reference to the global worker scope.
 * @type {WorkerGlobalScope}
 */
export const self = globalThis.self || globalThis

if (isWorkerScope) {
  // handle worker messages that are eventually propgated to `workerGlobalScopeEventTarget`
  globalThis.addEventListener('message', onWorkerMessage)
  globalThis.addEventListener('runtime-xhr-post-queue', function onXHRPostQueue (event) {
    if (isClosed) {
      globalThis.removeEventListener('runtime-xhr-post-queue', onXHRPostQueue)
      return false
    }

    const { id, seq, params } = event.detail || {}
    globals.get('RuntimeXHRPostQueue').dispatch(
      id,
      seq,
      params
    )
  })
}

let promise = null

if (source && typeof source === 'string') {
  promise = new Promise((resolve) => {
    // wait for everything to be ready, then import
    hooks.onReady(async () => {
      try {
        // @ts-ignore
        await import(source)
        if (Array.isArray(globalThis.RUNTIME_WORKER_MESSAGE_EVENT_BACKLOG)) {
          for (const event of globalThis.RUNTIME_WORKER_MESSAGE_EVENT_BACKLOG) {
            globalThis.dispatchEvent(new MessageEvent(event.type, event))
          }

          globalThis.RUNTIME_WORKER_MESSAGE_EVENT_BACKLOG.splice(
            0,
            globalThis.RUNTIME_WORKER_MESSAGE_EVENT_BACKLOG.length
          )
        }
        globalThis.postMessage({ __runtime_worker_init: true })
      } catch (err) {
        reportError(err)
      }

      resolve()
    })
  })
}

// overload worker event interfaces
Object.defineProperties(WorkerGlobalScopePrototype, {
  onmessage: {
    configurable: false,
    get: () => onmessage,
    set: (value) => {
      if (typeof onmessage === 'function') {
        workerGlobalScopeEventTarget.removeEventListener('message', onmessage)
      }

      if (value === null || typeof value === 'function') {
        onmessage = value
        workerGlobalScopeEventTarget.addEventListener('message', onmessage)
      }
    }
  },

  onconnect: {
    configurable: false,
    get: () => onconnect,
    set: (value) => {
      if (typeof onconnect === 'function') {
        workerGlobalScopeEventTarget.removeEventListener('connect', onconnect)
      }

      if (value === null || typeof value === 'function') {
        onconnect = value
        workerGlobalScopeEventTarget.addEventListener('connect', onconnect)
      }
    }
  },

  close: {
    configurable: false,
    enumerable: false,
    value: close
  },

  addEventListener: {
    configurable: false,
    enumerable: false,
    value: addEventListener
  },

  removeEventListener: {
    configurable: false,
    enumerable: false,
    value: removeEventListener
  },

  dispatchEvent: {
    configurable: false,
    enumerable: false,
    value: dispatchEvent
  },

  postMessage: {
    configurable: false,
    enumerable: false,
    value: postMessage
  },

  importScripts: {
    configurable: false,
    enumerable: false,
    value: importScripts
  }
})

export async function onWorkerMessage (event) {
  const { data } = event
  await promise

  if (typeof data?.__runtime_worker_ipc_result === 'object') {
    const resultData = data?.__runtime_worker_ipc_result || {}
    if (resultData.message && resultData.result) {
      const message = ipc.Message.from(resultData.message)
      const { index } = globalThis.__args
      const { seq } = message

      globalThis.dispatchEvent(new CustomEvent(`resolve-${index}-${seq}`, {
        detail: resultData.result
      }))
    }

    event.stopImmediatePropagation()
    return false
  } else if (typeof data?.__runtime_worker_event === 'object') {
    const { type, detail } = data?.__runtime_worker_event || {}
    if (type) {
      globalThis.dispatchEvent(new CustomEvent(type, { detail }))
    }
    event.stopImmediatePropagation()
    return false
  } else if (typeof data?.__runtime_shared_worker === 'object') {
    for (const port of data?.__runtime_shared_worker.ports) {
      if (ports.includes(port)) {
        ports.push(port)
      }
    }

    workerGlobalScopeEventTarget.dispatchEvent(new MessageEvent('connect', {
      ...data?.__runtime_shared_worker
    }))

    event.stopImmediatePropagation()
    return false
  }

  return dispatchEvent(new MessageEvent(event.type, event))
}

export function addEventListener (eventName, callback, ...args) {
  if (eventName === 'message' || eventName === 'connect') {
    return workerGlobalScopeEventTarget.addEventListener(eventName, callback, ...args)
  } else {
    return worker.addEventListener(eventName, callback, ...args)
  }
}

export function removeEventListener (eventName, callback, ...args) {
  if (eventName === 'message' || eventName === 'connect') {
    return workerGlobalScopeEventTarget.removeEventListener(eventName, callback, ...args)
  } else {
    return worker.removeEventListener(eventName, callback, ...args)
  }
}

export function dispatchEvent (event) {
  if (event.type === 'message' || event.type === 'connect') {
    return workerGlobalScopeEventTarget.dispatchEvent(event)
  }

  return worker.dispatchEvent(event)
}

export function postMessage (message, ...args) {
  return worker.postMessage(message, ...args)
}

export function close () {
  isClosed = true
  globalThis.removeEventListener('message', onWorkerMessage)

  for (const port of ports) {
    try {
      port.close()
    } catch {}
  }

  // release
  ports.splice(0, ports.length)

  return worker.close()
}

export function importScripts (...scripts) {
  const loader = new Loader(source)
  for (const script of scripts) {
    const { text, ok } = loader.load(script)
    if (ok && text) {
      // eslint-disable-next-line
      eval(text)
    }
  }
}

export default {
  RUNTIME_WORKER_ID,
  removeEventListener,
  addEventListener,
  importScripts,
  dispatchEvent,
  postMessage,
  source,
  close,
  url
}
