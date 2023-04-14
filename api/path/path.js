/**
 * @module Path
 *
 * Example usage:
 * ```js
 * import { Path } from 'socket:path'
 * ```
 */
import { primordials } from '../ipc.js'

const isWin32 = primordials.platform === 'win32'
const windowsPathRegex = /^(\/|\\)?[a-z]:/i

function maybeURL (...args) {
  try {
    return new URL(...args)
  } catch (_) {
    return null
  }
}

/**
 * @typedef {(string|Path|URL|{ pathname: string }|{ url: string)} PathComponent
 */

/**
 * A container for a parsed Path.
 */
export class Path extends URL {
  /**
   * Computes current working directory for a path
   * @param {object=} [opts]
   * @param {boolean=} [opts.posix] Set to `true` to force POSIX style path
   * @return {string}
   */
  static cwd (opts) {
    let cwd = primordials.cwd
    if (isWin32) {
      cwd = cwd.replace(windowsPathRegex, '')
      if (opts?.posix === true) {
        cwd = cwd.replace(/\\/g, '/')
        cwd = cwd.slice(cwd.indexOf('/'))
      }
    }

    return cwd.replace(/[/|\\]$/, '')
  }

  /**
   * Resolves path components to an absolute path.
   * @param {object} options
   * @param {...PathComponent} components
   * @return {string}
   */
  static resolve (options, ...components) {
    const { sep } = options
    let cwd = Path.cwd()
    while (components.length) {
      cwd = Path.from(components.shift(), cwd + sep)
        .pathname
        .replace(windowsPathRegex, '')
        .replace(/\/$/g, '')
        .replace(/\//g, sep)
    }

    if (cwd.endsWith(sep)) {
      cwd = cwd.slice(0, -1)
    }

    return cwd
  }

  /**
   * Computes the relative path from `from` to `to`.
   * @param {object} options
   * @param {PathComponent} from
   * @param {PathComponent} to
   * @return {string}
   */
  static relative (options, from, to) {
    const { sep } = options
    if (from === to) return ''
    from = this.resolve(options, from).replace('file:', '')
    to = this.resolve(options, to).replace('file:', '')

    const components = {
      output: [],
      from: from.split(sep).filter(Boolean),
      to: to.split(sep).filter(Boolean)
    }

    for (let i = components.from.length - 1; i > -1; --i) {
      const a = components.from[i]
      const b = components.to[i]
      if (a !== b) {
        components.output.push('..')
      }
    }

    for (let i = 0; i < components.output.length; ++i) {
      components.from.pop()
    }

    components.output.push(
      ...components.to
        .join(sep)
        .replace(components.from.join(sep), '')
        .split(sep)
        .filter(Boolean)
    )

    return components.output.join(sep)
      .replace(/^file:/g, '')
      .replace(windowsPathRegex, '')
  }

  /**
   * Joins path components. This function may not return an absolute path.
   * @param {object} options
   * @param {...PathComponent} components
   * @return {string}
   */
  static join (options, ...components) {
    const { sep } = options
    const joined = decodeURIComponent(
      components
        .filter((c) => c)
        .reduce((cwd, path) => Path.from(path, cwd + sep).pathname, '.')
        .replace(/\//g, sep)
    )

    if (!components[0]?.startsWith?.(sep)) {
      return joined.slice(1)
    }

    return joined
  }

  /**
   * Computes directory name of path.
   * @param {object} options
   * @param {...PathComponent} components
   * @return {string}
   */
  static dirname (options, path) {
    const { sep } = options
    path = path.replace('file://', '')
    let dirname = decodeURIComponent(
      Path
        .from(path, sep)
        .parent
        .replace(/\//g, sep)
        .replace(windowsPathRegex, '')
    )

    if (String(path).startsWith('.')) {
      dirname = `.${dirname}`
    } else if (!String(path).startsWith(sep)) {
      dirname = dirname.slice(1)
    }

    if (dirname.length > 1) {
      dirname = dirname.slice(0, -1)
    }

    return dirname || '.'
  }

  /**
   * Computes base name of path.
   * @param {object} options
   * @param {...PathComponent} components
   * @return {string}
   */
  static basename (options, path) {
    const { sep } = options
    const basename = decodeURIComponent(
      Path
        .from(path, sep)
        .base
        .replace(/\//g, sep)
    )

    return basename
  }

  /**
   * Computes extension name of path.
   * @param {object} options
   * @param {PathComponent} path
   * @return {string}
   */
  static extname (options, path) {
    const { sep } = options
    return Path.from(path, sep).ext
  }

  /**
   * Computes normalized path
   * @param {object} options
   * @param {PathComponent} path
   * @return {string}
   */
  static normalize (options, path) {
    path = String(path)

    const { drive, source, pathname } = Path.from(path, '.')
    const url = maybeURL(source) || {}
    const href = url.href ? url.href.replace(url.protocol, '') : ''
    const { sep } = options
    const prefix = drive || url.protocol || ''
    let output = prefix + Path
      .from(pathname.replace(windowsPathRegex, ''))
      .pathname
      .replace(/\//g, sep)
      .replace(/^file:/g, '')
      .replace(windowsPathRegex, '')

    if (url.protocol && href && !href.startsWith(sep)) {
      output = output.replace(url.protocol, '')

      if (output.startsWith(sep)) {
        output = output.slice(1)
      }

      if (!drive) {
        if (
          url.protocol !== 'file:' ||
          (url.protocol === 'file:' && path.startsWith('file:'))
        ) {
          output = url.protocol + output
        }
      }
    }

    if (path.endsWith(sep) && !output.endsWith(sep)) {
      output = output + sep
    }

    return output
  }

  /**
   * Formats `Path` object into a string.
   * @param {object|Path} path
   * @return {string}
   */
  static format (options, path) {
    const { root, dir, base, ext, name } = path
    const { sep } = options
    const output = []
    if (dir) output.push(dir, sep)
    else if (root) output.push(root)
    if (base) output.push(base)
    else if (name) output.push(name + (ext || ''))
    return output.join('').replace(/\//g, sep)
  }

  /**
   * Creates a `Path` instance from `input` and optional `cwd`.
   * @param {PathComponent} input
   * @param {string} [cwd]
   */
  static from (input, cwd) {
    if (typeof input?.href === 'string') {
      return this.from(input.href, cwd)
    } else if (typeof input?.pathname === 'string') {
      return this.from(input.pathname, cwd)
    }

    return new this(String(input || '.'), cwd ? String(cwd) : null)
  }

  #forwardSlashesDetected = false
  #isRelative = false
  #source = null
  #drive = null

  /**
   * `Path` class constructor.
   * @protected
   * @param {string} pathname
   * @param {string} [cwd = Path.cwd()]
   */
  constructor (pathname, cwd) {
    const isRelative = !/^[/|\\]/.test(pathname.replace(windowsPathRegex, ''))
    pathname = String(pathname || '.').trim()

    if (cwd) {
      cwd = new URL(`file://${cwd}`)
    } else if (pathname.startsWith('..')) {
      pathname = pathname.slice(2)
      cwd = 'file://..'
    } else if (isRelative) {
      cwd = new URL('file://.')
    } else {
      cwd = new URL(`file://${Path.cwd()}`)
    }

    super(pathname, cwd)

    const drive = (
      pathname.match(windowsPathRegex) || this.pathname.match(windowsPathRegex) || []
    )[0] || null

    this.#forwardSlashesDetected = /\\/.test(pathname)
    this.#isRelative = isRelative
    this.#source = pathname
    this.#drive = drive
  }

  /**
   * `true` if the path is relative, otherwise `false.
   * @type {boolean}
   */
  get isRelative () {
    return this.#isRelative
  }

  /**
   * The working value of this path.
   */
  get value () {
    return this.href.replace('file://', '')
  }

  /**
   * The original source, unresolved.
   * @type {string}
   */
  get source () {
    return this.#source
  }

  /**
   * Computed parent path.
   * @type {string}
   */
  get parent () {
    let i = this.pathname.lastIndexOf('/')
    if (i === -1) i = this.pathname.lastIndexOf('\\')
    return this.pathname
      .slice(0, i >= 0 ? i + 1 : undefined)
      .replace(windowsPathRegex, '')
  }

  /**
   * Computed root in path.
   * @type {string}
   */
  get root () {
    const { dir } = this
    const drive = (dir.match(windowsPathRegex) || [])[0] || this.drive

    if (!this.isRelative) {
      if (this.value.includes('\\') || this.#forwardSlashesDetected) {
        return `${drive || ''}\\`
      } else if (this.value.startsWith('/')) {
        return '/'
      }
    } else if (drive) {
      return drive
    }

    return ''
  }

  /**
   * Computed directory name in path.
   * @type {string}
   */
  get dir () {
    const { isRelative } = this
    let { parent } = this

    if (this.#forwardSlashesDetected) {
      parent = this.parent.replace(/\//g, '\\')
      if (parent.endsWith('\\') && parent.length > 1) {
        parent = parent.slice(0, -1)
      }
    } else {
      if (parent.endsWith('/') && parent.length > 1) {
        parent = parent.slice(0, -1)
      }
    }

    if (isRelative && (parent.startsWith('/') || parent.startsWith('\\'))) {
      parent = parent.slice(1)
    }

    if (this.drive) {
      return `${this.drive}${parent}`
    }

    if (this.#source.startsWith('./') || this.#source.startsWith('.\\')) {
      return `.${parent || ''}`
    }

    if (parent === '.') {
      return ''
    }

    return parent
  }

  /**
   * Computed base name in path.
   * @type {string}
   */
  get base () {
    let i = this.pathname.lastIndexOf('/')
    if (i === -1) {
      i = this.pathname.lastIndexOf('\\')
    }
    return this.pathname.slice(i >= 0 ? i + 1 : undefined)
  }

  /**
   * Computed base name in path without path extension.
   * @type {string}
   */
  get name () {
    return this.base.replace(this.ext, '')
  }

  /**
   * Computed extension name in path.
   * @type {string}
   */
  get ext () {
    let i = this.pathname.lastIndexOf('/')

    if (i === -1) {
      i = this.pathname.lastIndexOf('\\')
    }

    if (i === -1) return ''
    const pathname = this.pathname.slice(i)
    i = pathname.lastIndexOf('.')
    if (i === -1) return ''
    return pathname.slice(i >= 0 ? i : undefined)
  }

  /**
   * The computed drive, if given in the path.
   * @type {string?}
   */
  get drive () {
    return this.#drive
  }

  /**
   * @return {URL}
   */
  toURL () {
    return new URL(this.href.replace(/\\/g, '/'))
  }

  /**
   * Converts this `Path` instance to a string.
   * @return {string}
   */
  toString () {
    return decodeURIComponent(this.value)
  }

  /**
   * @ignore
   */
  inspect () {
    const p = this
    // eslint-disable-next-line new-parens
    return new class Path {
      root = p.root
      dir = p.dir
      base = p.base
      ext = p.ext
      name = p.name
    }
  }

  /**
   * @ignore
   */
  [Symbol.toStringTag] () {
    return 'Path'
  }
}

export default Path
