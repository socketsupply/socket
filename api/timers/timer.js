import gc from '../gc.js'

const platform = {
  setTimeout: globalThis.setTimeout.bind(globalThis),
  clearTimeout: globalThis.clearTimeout.bind(globalThis),
  setInterval: globalThis.setInterval.bind(globalThis),
  clearInterval: globalThis.clearInterval.bind(globalThis),
  setImmediate: globalThis.setTimeout.bind(globalThis),
  clearImmediate: globalThis.clearTimeout.bind(globalThis)
}

export class Timer {
  #id = 0
  #closed = false
  #create = null
  #destroy = null

  static from (...args) {
    const timer = new this()
    return timer.init(...args)
  }

  constructor (create, destroy) {
    if (typeof create !== 'function') {
      throw new TypeError('Timer creator must be a function')
    }

    if (typeof destroy !== 'function') {
      throw new TypeError('Timer destroyer must be a function')
    }

    this.#create = create
    this.#destroy = destroy
  }

  get id () {
    return this.#id
  }

  init (...args) {
    this.#id = this.create(...args)
    gc.ref(this)
  }

  close () {
    if (this.#id) {
      this.#destroy(this.#id)
      this.#closed = true
      this.#id = 0
      return true
    }

    return false
  }

  [Symbol.toPrimitive] () {
    return this.#id
  }

  [gc.finalizer] () {
    return {
      args: [this.id, this.#destroy],
      handle (id, destroy) {
        destroy(id)
      }
    }
  }
}

export class Timeout extends Timer {
  constructor () {
    super(
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
    super(platform.setInterval, platform.clearInterval)
  }
}

export class Immediate extends Timeout {}

export default {
  Timer,
  Immediate,
  Timeout,
  Interval
}
