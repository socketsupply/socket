/* global EventTarget, CustomEvent */
import './init.js'

import { rand64 } from '../crypto.js'
import globals from './globals.js'
import console from '../console.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

const WorkerGlobalScopePrototype = globalThis.WorkerGlobalScope?.prototype ?? {}

// level 1 worker `EventTarget` 'message' listener
let onmessage = null

// events are routed through this `EventTarget`
const eventTarget = new EventTarget()

// handle worker messages that are eventually propgated to `eventTarget`
globalThis.addEventListener('message', onWorkerMessage)
globalThis.addEventListener('runtime-xhr-post-queue', (event) => {
  const { id, seq, params } = event.detail
  globals.get('RuntimeXHRPostQueue').dispatch(
    id,
    seq,
    params
  )
})

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
export const source = globalThis.RUNTIME_WORKER_LOCATION || url.searchParams.get('source')

/**
 * A unique identifier for this worker made available on the global scope
 * @ignore
 * @type {string}
 */
export const RUNTIME_WORKER_ID = globalThis.RUNTIME_WORKER_ID || rand64().toString()

/**
 * Internally scoped event interface for a worker context.
 * @ignore
 * @type {object}
 */
export const worker = {
  postMessage: globalThis.postMessage.bind(globalThis),
  addEventListener: globalThis.addEventListener.bind(globalThis),
  removeEventListener: globalThis.removeEventListener.bind(globalThis),
  dispatchEvent: globalThis.dispatchEvent.bind(globalThis)
}

// wait for everything to be ready, then import
hooks.onReady(async () => {
  try {
    // @ts-ignore
    await import(source)
  } catch (err) {
    console.error(
      'RuntimeWorker: Failed to import worker:', err
    )
  }
})

// overload worker event interfaces
Object.defineProperties(WorkerGlobalScopePrototype, {
  onmessage: {
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
  }
})

export function onWorkerMessage (event) {
  const { data } = event

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
  }

  return dispatchEvent(event)
}

export function addEventListener (eventName, callback, ...args) {
  if (eventName === 'message') {
    return eventTarget.addEventListener(eventName, callback, ...args)
  } else {
    return worker.addEventListener(eventName, callback, ...args)
  }
}

export function removeEventListener (eventName, callback, ...args) {
  if (eventName === 'message') {
    return eventTarget.removeEventListener(eventName, callback, ...args)
  } else {
    return worker.removeEventListener(eventName, callback, ...args)
  }
}

export function dispatchEvent (event) {
  if (hooks.globalEvents.includes(event.type)) {
    return worker.dispatchEvent(event)
  }
  return eventTarget.dispatchEvent(event)
}

export function postMessage (message, ...args) {
  return worker.postMessage(message, ...args)
}

export default {
  RUNTIME_WORKER_ID,
  removeEventListener,
  addEventListener,
  dispatchEvent,
  postMessage,
  source,
  url
}
