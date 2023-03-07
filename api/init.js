/* global CustomEvent */
/* eslint-disable import/first */
globalThis.__RUNTIME_INIT_NOW__ = performance.now()

import { IllegalConstructor, InvertedPromise } from 'socket:util'
import { applyPolyfills } from 'socket:polyfills'
import hooks from 'socket:hooks'
import ipc from 'socket:ipc'

// async preload modules
hooks.onReady(async () => {
  // precache fs.constants
  await ipc.request('fs.constants', {}, { cache: true })
  import('socket:diagnostics')
  import('socket:fs/fds')
  import('socket:fs/constants')
})

class RuntimeXHRPostQueue extends EventTarget {
  concurrency = 16
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

globalThis.__RUNTIME_XHR_POST_QUEUE__ = new RuntimeXHRPostQueue()
// prevent further construction if this class is indirectly referenced
RuntimeXHRPostQueue.prototype.constructor = IllegalConstructor
export default applyPolyfills()
