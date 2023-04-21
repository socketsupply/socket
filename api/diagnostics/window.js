import { IllegalConstructor } from '../util.js'
import { Worker } from '../worker.js'
import { Metric } from './metric.js'
import registry from './channels.js'
import process from '../process.js'

const dc = registry.group('window', [
  'XMLHttpRequest.open',
  'XMLHttpRequest.send',
  'Worker',
  'fetch',
  'fetch.start',
  'fetch.end',
  'requestAnimationFrame'
])

export class RequestAnimationFrameMetric extends Metric {
  constructor (options) {
    super()
    this.originalRequestAnimationFrame = null
    this.requestAnimationFrame = this.requestAnimationFrame.bind(this)
    this.sampleSize = options?.sampleSize || 60
    this.sampleTick = 0 // max(0, sampleSize)
    this.channel = dc.channel('requestAnimationFrame')
    this.value = null
    this.now = 0
  }

  requestAnimationFrame (callback) {
    return this.originalRequestAnimationFrame.call(globalThis, (...args) => {
      this.update(...args)
      // eslint-disable-next-line n/no-callback-literal
      callback(...args)
    })
  }

  init () {
    if (
      this.originalRequestAnimationFrame === null &&
      typeof globalThis.requestAnimationFrame === 'function'
    ) {
      this.samples = new Uint8Array(this.sampleSize)
      this.originalRequestAnimationFrame = globalThis.requestAnimationFrame
      globalThis.requestAnimationFrame = this.requestAnimationFrame

      process.once('exit', () => {
        this.destroy()
      })
    }
  }

  update (value) {
    const then = this.now
    const now = globalThis?.performance.now()

    if (then > 0) {
      const computed = 1 / (0.001 * (now - then))
      const sample = Math.floor(computed)

      if (this.samples) {
        this.samples[this.sampleTick] = sample
      }

      this.sampleTick = (this.sampleTick + 1) % this.sampleSize

      const tick = this.sampleTick
      const sum = this.samples?.slice(0, tick).reduce((a, b) => a + b, 0)
      const average = sum / tick
      const rate = Math.round(average)
      const metric = { rate, samples: tick - 1 }

      this.value = metric

      this.channel?.publish(metric)
    }

    this.now = now
  }

  destroy () {
    this.channel.reset()

    if (typeof this.originalRequestAnimationFrame === 'function') {
      globalThis.requestAnimationFrame = this.originalRequestAnimationFrame
    }

    this.originalRequestAnimationFrame = null
    this.samples = null
    this.value = null
  }

  toJSON () {
    return {
      sampleSize: this.sampleSize,
      sampleTick: this.sampleTick,
      samples: Array.from(this.samples),
      rate: this.value?.rate ?? 0,
      now: this.now
    }
  }
}

export class FetchMetric extends Metric {
  constructor (options) {
    super()
    this.originalFetch = null
    this.channel = dc.channel('fetch')
    this.fetch = this.fetch.bind(this)
  }

  async fetch (resource, options, extra) {
    const metric = { resource, options, extra }
    this.channel.channel('start').publish(metric)

    const args = [resource, options, extra]
    const response = await this.originalFetch.apply(globalThis, args)
    metric.response = response.clone()

    this.channel.channel('end').publish(metric)
    this.channel.publish(metric)
    return response
  }

  init () {
    if (!this.originalFetch) {
      this.originalFetch = globalThis.fetch
      globalThis.fetch = this.fetch
    }
  }

  destroy () {
    this.channel.reset()

    if (typeof this.originalFetch === 'function') {
      globalThis.fetch = this.originalFetch
    }

    this.originalFetch = null
  }
}

export class XMLHttpRequestMetric extends Metric {
  constructor (options) {
    super()
    this.channel = dc.channel('XMLHttpRequest')
    this.patched = null
  }

  init () {
    const { channel } = this

    if (!this.patched) {
      const { open, send } = globalThis.XMLHttpRequest.prototype
      this.patched = { open, send }

      Object.assign(globalThis.XMLHttpRequest.prototype, {
        open (...args) {
          const [method, url, async] = args
          channel.channel('open').publish({ request: this, method, url, async })
          return open.call(this, ...args)
        },

        send (...args) {
          const [body] = args
          channel.channel('send').publish({ request: this, body })
          return send.call(this, ...args)
        }
      })
    }
  }

  destroy () {
    this.channel.reset()

    if (this.patched) {
      Object.assign(globalThis.XMLHttpRequest.prototype, this.patched)
    }

    this.patched = null
  }
}

export class WorkerMetric extends Metric {
  /**
   * @type {Worker}
   */
  static GlobalWorker = Worker

  constructor (options) {
    super()
    this.channel = dc.channel('Worker')
    this.Worker = class Worker extends WorkerMetric.GlobalWorker {
      constructor (url, options, ...args) {
        super(url, options, ...args)
        dc.channel('Worker').publish({ worker: this, url, options })
      }
    }
  }

  init () {
    globalThis.Worker = this.Worker
  }

  destroy () {
    this.channel.reset()
    globalThis.Worker = WorkerMetric.GlobalWorker
  }
}

// eslint-disable-next-line new-parens
export const metrics = new class Metrics {
  // metrics
  requestAnimationFrame = new RequestAnimationFrameMetric()
  XMLHttpRequest = new XMLHttpRequestMetric()
  Worker = new WorkerMetric()
  fetch = new FetchMetric()

  channel = dc

  subscribe (...args) {
    return dc.subscribe(...args)
  }

  unsubscribe (...args) {
    return dc.unsubscribe(...args)
  }

  start (which) {
    if (Array.isArray(which)) {
      for (const key of which) {
        if (typeof this[key]?.init === 'function') {
          this[key].init()
        }
      }
    } else {
      for (const value of Object.values(this)) {
        if (typeof value?.init === 'function') {
          value.init()
        }
      }
    }
  }

  stop (which) {
    if (Array.isArray(which)) {
      for (const key of which) {
        if (typeof this[key]?.destroy === 'function') {
          this[key].destroy()
        }
      }
    } else {
      for (const value of Object.values(this)) {
        if (typeof value?.destroy === 'function') {
          value.destroy()
        }
      }
    }
  }
}

// make construction illegal
Object.assign(Object.getPrototypeOf(metrics), {
  constructor: IllegalConstructor
})

export default { metrics }
