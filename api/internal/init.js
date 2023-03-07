/* global CustomEvent */
/* eslint-disable import/first */
globalThis.__RUNTIME_INIT_NOW__ = performance.now()

import { IllegalConstructor, InvertedPromise } from '../util.js'
import { applyPolyfills } from '../polyfills.js'
import globals from './globals.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

// async preload modules
hooks.onReady(async () => {
  // precache fs.constants
  await ipc.request('fs.constants', {}, { cache: true })
  import('../diagnostics.js')
  import('../fs/fds.js')
  import('../fs/constants.js')
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

globals.register('RuntimeXHRPostQueue', new RuntimeXHRPostQueue())
// prevent further construction if this class is indirectly referenced
RuntimeXHRPostQueue.prototype.constructor = IllegalConstructor
export default applyPolyfills()
