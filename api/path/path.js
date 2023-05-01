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
const protocolRegex = /(^[a-z]+:(\/\/)?)/i
const protocolStrictSlashesRegex = /(^[a-z]+:\/\/)/i
const windowsDriveRegex = /^[a-z]:/i
const windowsDriveInPathRegex = /^\/[a-z]:/i

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
      cwd = cwd.replace(windowsDriveRegex, '')
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
      const path = Path.from(components.shift(), cwd + sep)
      cwd = path.value
        .replace(/\//g, sep)
        .replace(new RegExp(`/${sep}$/`, 'g'), '')

      if (!path.drive) {
        cwd = cwd.replace(windowsDriveRegex, '')
      }
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

    const relative = components.output.join(sep).replace(/^file:/g, '')

    if (windowsDriveRegex.test(from) || windowsDriveRegex.test(to)) {
      return relative.replace(windowsDriveRegex, '')
    }

    return relative
  }

  /**
   * Joins path components. This function may not return an absolute path.
   * @param {object} options
   * @param {...PathComponent} components
   * @return {string}
   */
  static join (options, ...components) {
    const { sep } = options
    const queries = []
    const joined = []

    while (components.length) {
      const component = String(components.shift() || '')
      const parts = component.split(sep)
      while (parts.length) {
        queries.push(parts.shift())
      }
    }

    for (const query of queries) {
      if (query === '..' && joined.length > 1 && joined[0] !== '..') {
        joined.pop()
      } else if (query !== '.') {
        joined.push(query)
      }
    }

    return joined.join(sep)
  }

  /**
   * Computes directory name of path.
   * @param {object} options
   * @param {...PathComponent} components
   * @return {string}
   */
  static dirname (options, path) {
    const { sep } = options
    const protocol = path.startsWith('file://')
      ? 'file://'
      : path.startsWith('file:') ? 'file:' : ''

    path = path.replace('file://', '')

    if (isWin32 && windowsDriveInPathRegex.test(path)) {
      path = path.slice(1)
    }

    const pathWithoutDrive = path.replace(/^[a-z]:\\/i, '')
    const p = Path.from(path, sep)
    let dirname = decodeURIComponent(
      p.parent
        .replace(/\//g, sep)
        .replace(windowsDriveRegex, '')
        .replace(windowsDriveInPathRegex, '')
    )

    if (String(pathWithoutDrive).startsWith('.')) {
      let tmp = ''
      tmp += p.drive ? p.drive + sep : ''
      if (!dirname.startsWith(sep)) tmp += sep
      if (!dirname.startsWith(sep + '.')) tmp += '.'
      dirname = dirname.replace(/^[a-z]:\\/i, '').replace(/^(\/|\\)/, '')
      if (tmp.endsWith('.') && !dirname.startsWith(sep)) {
        tmp += sep
      }
      tmp += dirname
      dirname = tmp
    } else if (
      !windowsDriveRegex.test(dirname) &&
      !protocolRegex.test(path) &&
      !path.startsWith(sep)
    ) {
      dirname = dirname.slice(1)
    }

    const dirnameWithoutDrive = dirname.replace(windowsDriveRegex, '')
    if (
      !path.endsWith(sep) &&
      dirname.endsWith(sep) &&
      dirnameWithoutDrive.length > 1
    ) {
      dirname = dirname.slice(0, -1)
    }

    if (p.drive) {
      if (!windowsDriveRegex.test(dirname) && !windowsDriveInPathRegex.test(dirname)) {
        if (dirname.startsWith(sep)) {
          dirname = p.drive + dirname
        } else {
          dirname = p.drive + sep + dirname
        }
      }
    }

    if (protocol && p.drive) {
      dirname = '/' + dirname
    }

    return protocol + dirname || '.'
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
      Path.from(path, sep).base.replace(/\//g, sep)
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
    const href = url.href ? url.href.replace(protocolRegex, '') : ''
    const { sep } = options
    const prefix = drive || url.protocol || ''
    const p = Path.from(pathname)
    let output = prefix + p.value
      .replace(protocolRegex, '')
      .replace(/\//g, sep)

    if (drive && !windowsDriveRegex.test(output)) {
      output = drive + sep + output
    }

    if (url.protocol && href && !href.startsWith(sep)) {
      output = output.replace(url.protocol, '')
      if (output.startsWith(sep) && !protocolStrictSlashesRegex.test(path)) {
        output = output.slice(1)
      }

      if (!drive) {
        if (
          url.protocol !== 'file:' ||
          (url.protocol === 'file:' && path.startsWith('file:'))
        ) {
          if (protocolStrictSlashesRegex.test(path)) {
            output = url.protocol + '//' + url.hostname + output
          } else {
            output = url.protocol + url.hostname + output
          }
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
   * @param {object} options
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
   * Parses input `path` into a `Path` instance.
   * @param {PathComponent} path
   * @return {object}
   */
  static parse (path) {
    const sep = /\\/.test(path) ? '\\' : '/'
    const [drive] = path.match(windowsDriveRegex) || []
    const parsed = { root: '', dir: '', base: '', ext: '', name: '' }
    const parts = path.replace(windowsDriveRegex, '').split(sep)

    parsed.root = !parts[0] ? parts[0] || sep : ''
    parsed.dir = !parts[0] || parts[0].startsWith('.')
      ? parts.slice(0, -1).join(sep)
      : [parsed.root].concat(parts.slice(1).slice(0, -1)).join(sep)
    parsed.ext = this.extname({ sep }, path)
    parsed.base = this.basename({ sep }, path)
    parsed.name = parsed.base.replace(parsed.ext, '')

    if (!parsed.dir) parsed.dir = parsed.root
    if (drive && parsed.root) parsed.root = drive + parsed.root
    if (drive && parsed.dir) parsed.dir = drive + parsed.dir

    return parsed
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
    const isRelative = !/^[/|\\]/.test(pathname.replace(windowsDriveRegex, ''))
    pathname = String(pathname || '.').trim()

    if (cwd) {
      cwd = new URL(`file://${cwd.replace('file://', '')}`)
    } else if (pathname.startsWith('..')) {
      pathname = pathname.slice(2)
      cwd = 'file://..'
    } else if (isRelative) {
      cwd = new URL('file://.')
    } else {
      cwd = new URL(`file://${Path.cwd()}`)
    }

    super(pathname, cwd)

    const [drive] = (
      pathname.match(windowsDriveRegex) ||
      this.pathname.match(windowsDriveRegex) ||
      []
    )

    this.#forwardSlashesDetected = /\\/.test(pathname)
    this.#isRelative = isRelative
    this.#source = pathname
    this.#drive = drive ? drive.replace(/(\\|\/)/g, '') : null
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
    const { protocol, drive } = this
    const regex = new RegExp(`^${protocol}(//)?(.[\\|/])?`, 'i')
    const href = this.href.replace(regex, '')
    return !drive ? href.replace(windowsDriveInPathRegex, '') : href
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
    let pathname = this.value
    let i = pathname.lastIndexOf('/')
    if (i === -1) i = pathname.lastIndexOf('\\')
    if (i >= 0) pathname = pathname.slice(0, i + 1)
    else pathname = ''

    if (this.drive) {
      return this.drive + pathname.replace(windowsDriveInPathRegex, '')
    }

    return pathname
  }

  /**
   * Computed root in path.
   * @type {string}
   */
  get root () {
    const { dir } = this
    const drive = (dir.match(windowsDriveRegex) || [])[0] || this.drive

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
      parent = parent.replace(/\//g, '\\')
    } else {
      if (parent.endsWith('/') && parent.length > 1) {
        parent = parent.slice(0, -1)
      }
    }

    if (isRelative && (parent.startsWith('/') || parent.startsWith('\\'))) {
      parent = parent.slice(1)
    }

    if (this.drive) {
      return this.drive + parent.replace(windowsDriveRegex, '')
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
