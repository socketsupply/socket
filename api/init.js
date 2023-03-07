/* global CustomEvent */
import { InvertedPromise } from 'socket:util'
import ipc from 'socket:ipc'

// eslint-disable-next-line new-parens
globalThis.__CORE_XHR_POST_QUEUE__ = new class CoreXHRPostQueue extends EventTarget {
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

    if (result.srr) {
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

export default undefined
