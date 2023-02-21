import { format, isArrayLike, IllegalConstructor } from '../util.js'
import registry from './channels.js'
import process from '../process.js'

const dc = registry.group('window', [
  'XMLHttpRequest',
  'XMLHttpRequest.open',
  'XMLHttpRequest.send',
  'worker',
  'fetch',
  'fetch.start',
  'fetch.end',
  'requestAnimationFrame'
])

export class Metric {
  init () {}
  update () {}
  destroy () {}

  toJSON () {
    return {}
  }

  toString () {
    return format(this)
  }

  [Symbol.iterator] () {
    if (isArrayLike(this)) {
      return Array.from(this)
    }

    return [this]
  }

  [Symbol.toStringTag] () {
    const name = this.constructor.name.replace('Metric', '')
    return `DiagnosticMetric(${name})`
  }
}

export class AnimationFrameMetric extends Metric {
  constructor (options) {
    super()
    this.originalRequestAnimationFrame = null
    this.requestAnimationFrame = this.requestAnimationFrame.bind(this)
    this.sampleSize = options?.sampleSize || 60
    this.channel = dc.channel('requestAnimationFrame')
    this.frame = null
    this.tick = 0 // max(0, sampleSize)
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
    if (this.frame === null) {
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
        this.samples[this.tick] = sample
      }

      this.tick = (this.tick + 1) % this.sampleSize

      const sum = this.samples?.slice(0, this.tick).reduce((a, b) => a + b, 0)
      const average = sum / (this.tick)
      const rate = Math.round(average)
      const metric = { rate, samples: this.tick - 1 }

      this.channel?.publish(metric)
    }

    this.now = now
  }

  destroy () {
    globalThis.cancelAnimationFrame(this.frame)

    if (typeof this.originalRequestAnimationFrame === 'function') {
      globalThis.requestAnimationFrame = this.originalRequestAnimationFrame
    }

    this.originalRequestAnimationFrame = null
    this.samples = null
    this.channel = null
    this.frame = null
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
    if (typeof this.originalFetch === 'function') {
      globalThis.fetch = this.originalFetch
      this.originalFetch = null
    }
  }
}

export class XMLHttpRequestMetric extends Metric {
  constructor (options) {
    super()
    const self = this
    this.channel = dc.channel('xmlhttprequest')
    this.OrigialXMLHttpRequest = null

    this.XMLHttpRequest = class XMLHttpRequest extends globalThis.XMLHttpRequest {
      constructor () {
        super()
        self.channel.publish({ request: this })
      }

      send (body = null, ...args) {
        self.channel.channel('send').publish({ request: this, body })
        return self.OrigialXMLHttpRequest.prototype.send.call(this, body, ...args)
      }

      open (method, url, async, ...args) {
        self.channel.channel('open').publish({ request: this, method, url, async })
        return self.OrigialXMLHttpRequest.prototype.open.call(this, method, url, ...args)
      }
    }
  }

  init () {
    if (!this.OrigialXMLHttpRequest) {
      this.OrigialXMLHttpRequest = globalThis.XMLHttpRequest
      globalThis.XMLHttpRequest = this.XMLHttpRequest
    }
  }

  destroy () {
    if (typeof this.OrigialXMLHttpRequest === 'function') {
      globalThis.XMLHttpRequest = this.OrigialXMLHttpRequest
      this.OrigialXMLHttpRequest = null
    }
  }
}

// eslint-disable-next-line new-parens
export const metrics = new class Metrics {
  animationFrame = new AnimationFrameMetric()
  XMLHttpRequest = new XMLHttpRequestMetric()
  fetch = new FetchMetric()

  channel = dc

  subscribe (...args) {
    return dc.subscribe(...args)
  }

  unsubscribe (...args) {
    return dc.unsubscribe(...args)
  }

  init (which) {
    if (Array.isArray(which)) {
      for (const key of which) {
        if (typeof this[key]?.init === 'function') {
          this[key].init()
        }
      }
    } else {
      this.animationFrame.init()
      this.XMLHttpRequest.init()
      this.fetch.init()
    }
  }
}

// make construction illegal
Object.assign(Object.getPrototypeOf(metrics), {
  constructor: IllegalConstructor
})

export default { metrics }
