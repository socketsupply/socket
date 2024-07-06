/* eslint-disable import/first */
globalThis.isSharedWorkerScope = true

import { SharedWorkerMessagePort, channel } from './index.js'
import { SharedWorkerGlobalScope } from './global.js'
import { Module, createRequire } from '../module.js'
import { Environment } from '../service-worker/env.js'
import { Buffer } from '../buffer.js'
import { Cache } from '../commonjs/cache.js'
import globals from '../internal/globals.js'
import process from '../process.js'
import debug from './debug.js'
import hooks from '../hooks.js'
import state from './state.js'
import path from '../path.js'
import ipc from '../ipc.js'

import '../console.js'

export default null

Object.defineProperties(
  globalThis,
  Object.getOwnPropertyDescriptors(SharedWorkerGlobalScope.prototype)
)

export const SHARED_WORKER_READY_TOKEN = { __shared_worker_ready: true }

// state
export const module = { exports: {} }

// event listeners
hooks.onReady(onReady)
globalThis.addEventListener('message', onMessage)

// shared worker globals
globals.register('SharedWorker.state', state)
globals.register('SharedWorker.module', module)

export function onReady () {
  globalThis.postMessage(SHARED_WORKER_READY_TOKEN)
}

export async function onMessage (event) {
  const { data } = event

  if (data?.install) {
    event.stopImmediatePropagation()

    const { id, scriptURL } = data.install
    const url = new URL(scriptURL)

    if (!url.pathname.startsWith('/socket/')) {
      // preload commonjs cache for user space server workers
      Cache.restore(['loader.status', 'loader.response'])
    }

    state.id = id
    state.sharedWorker.id = id
    state.sharedWorker.scriptURL = scriptURL

    Module.main.addEventListener('error', (event) => {
      if (event.error) {
        debug(event.error)
      }
    })

    Object.defineProperties(globalThis, {
      require: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: createRequire(scriptURL)
      },

      origin: {
        configurable: false,
        enumerable: true,
        writable: false,
        value: url.origin
      },

      __dirname: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: path.dirname(url.pathname)
      },

      __filename: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: url.pathname
      },

      module: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: module
      },

      exports: {
        configurable: false,
        enumerable: false,
        get: () => module.exports
      },

      process: {
        configurable: false,
        enumerable: false,
        get: () => process
      },

      Buffer: {
        configurable: false,
        enumerable: false,
        get: () => Buffer
      },

      global: {
        configurable: false,
        enumerable: false,
        get: () => globalThis
      }
    })

    try {
      // define the actual location of the worker. not `blob:...`
      globalThis.RUNTIME_WORKER_LOCATION = scriptURL
      // update state
      state.sharedWorker.state = 'installing'
      // open envirnoment
      state.env = await Environment.open({ type: 'sharedWorker', id })
        console.log({ scriptURL })
      // import module, which could be ESM, CommonJS,
      // or a simple SharedWorker
      const result = await import(scriptURL)

      if (typeof module.exports === 'function') {
        module.exports = {
          default: module.exports
        }
      } else {
        Object.assign(module.exports, result)
      }
      // update state
      state.sharedWorker.state = 'installed'
    } catch (err) {
      debug(err)
      state.sharedWorker.state = 'error'
      channel.postMessage({
        error: { id: state.id, message: err.message }
      })
      return
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.connect === 'function') {
        state.connect = module.exports.default.connect.bind(module.exports.default)
      }
    } else if (typeof module.exports.connect === 'function') {
      state.connect = module.exports.connect.bind(module.exports)
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.reportError === 'function') {
        state.reportError = module.exports.default.reportError.bind(module.exports.default)
      }
    } else if (typeof module.exports.reportError === 'function') {
      state.reportError = module.reportError.bind(module.exports)
    }

    if (typeof state.connect === 'function') {
      globalThis.addEventListener('connect', async (event) => {
        try {
          const promise = state.connect(state.env, event.data, event.ports[0])
          await promise
        } catch (err) {
          debug(err)
          channel.postMessage({
            error: { id: state.id, message: err.message }
          })
        }
      })
    }
    channel.postMessage({ installed: { id: state.id } })
    return
  }

  if (data?.connect) {
    event.stopImmediatePropagation()
    const connection = ipc.inflateIPCMessageTransfers(event.data.connect, new Map(Object.entries({
      'SharedWorkerMessagePort': SharedWorkerMessagePort
    })))

    const connectEvent = new MessageEvent('connect')
    Object.defineProperty(connectEvent, 'ports', {
      configurable: false,
      writable: false,
      value: Object.seal(Object.freeze([connection.port]))
    })
      console.log({ connectEvent })
    globalThis.dispatchEvent(connectEvent)
    // eslint-disable-next-line
    return
  }
}
