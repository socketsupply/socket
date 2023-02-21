import { isArrayLike, format } from '../util.js'

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

export default Metric
