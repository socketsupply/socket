/**
 * @module Path
 */
import process from '../process.js'
import os from '../os.js'

const isWin32 = os.platform() === 'win32'

export class Path {
  /**
   * Computes current working directory for a path
   * @param {object=} [opts]
   * @param {boolean=} [opts.posix] Set to `true` to force POSIX style path
   */
  static cwd (opts) {
    if (isWin32 && opts?.posix === true) {
      const cwd = process.cwd().replace(/\\/g, '/')
      return cwd.slice(cwd.indexOf('/'))
    }

    return process.cwd()
  }

  static from (input) {
    if (typeof input === 'string') {
      return new this(this.parse(input))
    } else if (input && typeof input === 'object') {
      return new this(input)
    }

    throw new TypeError('Invalid input given to `Path.from()`')
  }

  /**
   * `Path` class constructor.
   * @protected
   * @param {object=} [opts]
   * @param {string=} [opts.root]
   * @param {string=} [opts.base]
   * @param {string=} [opts.name]
   * @param {string=} [opts.dir]
   * @param {string=} [opts.ext]
   */
  constructor (opts) {
    this.root = opts?.root ?? ''
    this.base = opts?.base ?? ''
    this.name = opts?.name ?? ''
    this.dir = opts?.dir ?? ''
    this.ext = opts?.ext ?? ''
  }

  toString () {
    const { format } = this.constructor
    return format(this)
  }

  /**
   * @TODO
   */
  static resolve (...args) {
  }

  /**
   * @TODO
   */
  static normalize (path) {
  }
}
