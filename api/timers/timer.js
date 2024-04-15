import { AsyncResource } from '../async/resource.js'
import platform from './platform.js'
import gc from '../gc.js'

export class Timer extends AsyncResource {
  #id = 0
  #create = null
  #destroy = null
  #destroyed = false

  static from (...args) {
    const timer = new this()
    return timer.init(...args)
  }

  constructor (type, create, destroy) {
    super(type, { requireManualDestroy: true })

    if (typeof create !== 'function') {
      throw new TypeError('Timer creator must be a function')
    }

    if (typeof destroy !== 'function') {
      throw new TypeError('Timer destroyer must be a function')
    }

    this.#create = (callback, ...args) => {
      return create(() => this.runInAsyncScope(callback), ...args)
    }

    this.#destroy = (...args) => {
      if (typeof destroy === 'function') {
        destroy(...args)
      }

      this.#destroyed = true
      this.emitDestroy()
    }
  }

  get id () {
    return this.#id
  }

  get destroyed () {
    return this.#destroyed
  }

  init (...args) {
    this.#id = this.#create(...args)
    gc.ref(this)
    return this
  }

  close () {
    if (this.#id) {
      this.#destroy(this.#id)
      this.#id = 0
      return true
    }

    return false
  }

  [Symbol.toPrimitive] () {
    return this.#id
  }

  [gc.finalizer] () {
    const finalizer = super[gc.finalizer] ? super[gc.finalizer]() : null
    return {
      args: [this.id, this.#destroy],
      handle (id, destroy) {
        destroy(id)
        if (finalizer?.handle && finalizer?.args) {
          finalizer.handle(...finalizer.args)
        } else if (finalizer?.handle) {
          finalizer.handle()
        }
      }
    }
  }
}

export class Timeout extends Timer {
  constructor () {
    super(
      'Timeout',
      (callback, delay, ...args) => {
        return platform.setTimeout(
          (...args) => {
            this.close()
            // eslint-disable-next-line
            callback(...args)
          },
          delay,
          ...args
        )
      },
      platform.clearTimeout
    )
  }
}

export class Interval extends Timer {
  constructor () {
    super('Interval', platform.setInterval, platform.clearInterval)
  }
}

export class Immediate extends Timer {
  constructor () {
    super(
      'Immediate',
      (callback, _, ...args) => {
        return platform.setImmediate(
          (...args) => {
            this.close()
            // eslint-disable-next-line
            callback(...args)
          },
          0,
          ...args
        )
      },
      platform.clearImmediate
    )
  }
}

export default {
  Timer,
  Immediate,
  Timeout,
  Interval
}
