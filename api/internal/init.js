/* global CustomEvent, Blob */
/* eslint-disable import/first */
globalThis.__RUNTIME_INIT_NOW__ = performance.now()

import { IllegalConstructor, InvertedPromise } from '../util.js'
import { applyPolyfills } from '../polyfills.js'
import globals from './globals.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

class RuntimeXHRPostQueue extends EventTarget {
  concurrency = 4 * (globalThis?.navigator?.hardwareConcurrency || 4)
  pending = []

  async wait () {
    if (this.pending.length < this.concurrency) return
    await this.peek()
  }

  peek () {
    return this.pending[0]
  }

  async dispatch (id, seq, params, headers) {
    await this.wait()
    const promise = new InvertedPromise()
    this.pending.push(promise)

    if (typeof params !== 'object') {
      params = {}
    }

    params = { ...params, id }
    const options = { responseType: 'arraybuffer' }
    const result = await ipc.request('post', params, options)
    const index = this.pending.indexOf(promise)

    if (index >= 0) {
      this.pending.splice(index, 1)
    }

    if (result.err) {
      this.dispatchEvent(new CustomEvent('error', { detail: result.err }))
      promise.resolve()
      return
    }

    queueMicrotask(() => {
      const { data } = result
      const detail = { headers, params, data, id }
      globalThis.dispatchEvent(new CustomEvent('data', { detail }))
      promise.resolve()
    })
  }
}

if (typeof globalThis?.Worker === 'function' && globalThis.window) {
  // async preload modules
  hooks.onReady(async () => {
    // precache fs.constants
    await ipc.request('fs.constants', {}, { cache: true })
    import('../diagnostics.js')
    import('../fs/fds.js')
    import('../fs/constants.js')
  })

  globalThis.Worker = class Worker extends globalThis?.Worker {
    #onmessage = null

    constructor (url, options, ...args) {
      const preload = `
      globalThis.__args = ${JSON.stringify(globalThis.__args)}
      await import('socket:internal/init')
      await import('socket:internal/worker')
      await import('${new URL(url, globalThis.location?.href || '/')}')
      `

      const blob = new Blob([preload], { type: 'application/javascript' })
      const uri = URL.createObjectURL(blob)

      super(uri, { ...options, type: 'module' }, ...args)

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
                const message = ipc.Message.from(request.message, request.bytes)
                const options = { bytes: message.bytes }
                const result = await ipc.send(message.name, message.rawParams, options)
                this.postMessage({
                  __runtime_worker_ipc_result: JSON.parse(JSON.stringify({
                    message,
                    result
                  }))
                })
              } catch (err) {
                console.warn('__runtime_worker_ipc_result:', err.stack || err)
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
  }
}

globals.register('RuntimeXHRPostQueue', new RuntimeXHRPostQueue())
// prevent further construction if this class is indirectly referenced
RuntimeXHRPostQueue.prototype.constructor = IllegalConstructor
export default applyPolyfills()
