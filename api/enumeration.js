export class Enumeration extends Set {
  static from (...values) {
    if (values.length > 1 && typeof values[1] !== 'object') {
      return new this(values)
    }

    return new this(...values)
  }

  constructor (values, options = {}) {
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
        value: value
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
