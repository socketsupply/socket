/**
 * @module enumeration
 * This module provides a data structure for enumerated unique values.
 */

/**
 * A container for enumerated values.
 */
export class Enumeration extends Set {
  /**
   * Creates an `Enumeration` instance from arguments.
   * @param {...any} values
   * @return {Enumeration}
   */
  static from (...values) {
    if (values.length > 1 && typeof values[1] !== 'object') {
      return new this(values)
    }

    return new this(...values)
  }

  /**
   * `Enumeration` class constructor.
   * @param {any[]} values
   * @param {object=} [options = {}]
   * @param {number=} [options.start = 0]
   */
  constructor (values, options = { start: 0 }) {
    super()

    let index = options?.start ?? 0
    for (const value of values) {
      if (this.has(value)) continue

      if (typeof value !== 'string' && typeof value !== 'number') {
        throw new TypeError(
          'Failed to construct \'Enumeration\': ' +
          'Invalid enumerable value given.')
      }

      Object.defineProperty(this, value, {
        configurable: false,
        enumerable: true,
        writable: false,
        value: index
      })

      Object.defineProperty(this, index, {
        configurable: false,
        enumerable: false,
        writable: false,
        value
      })

      Set.prototype.add.call(this, value)
      index++
    }

    Object.freeze(this)
    Object.seal(this)
  }

  /**
   * @ignore
   * @type {string}
   */
  get [Symbol.toStringTag] () {
    return this.constructor.name
  }

  /**
   * @type {number}
   */
  get length () {
    return this.size
  }

  /**
   * Returns `true` if enumeration contains `value`. An alias
   * for `Set.prototype.has`.
   * @return {boolean}
   */
  contains (value) {
    return this.has(value)
  }

  /**
   * @ignore
   */
  add () {}

  /**
   * @ignore
   */
  delete () {}

  /**
   * JSON represenation of a `Enumeration` instance.
   * @ignore
   * @return {string[]}
   */
  toJSON () {
    return Array.from(this)
  }

  /**
   * Internal inspect function.
   * @ignore
   * @return {LanguageQueryResult}
   */
  inspect () {
    const tag = this[Symbol.toStringTag]
    return `${tag} { ${this.toJSON().join(', ')} }`
  }

  /**
   * @ignore
   */
  [Symbol.for('socket.util.inspect.custom')] () {
    return this.inspect()
  }

  /**
   * @ignore
   */
  [Symbol.for('nodejs.util.inspect.custom')] () {
    return this.inspect()
  }
}

export default Enumeration
