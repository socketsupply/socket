/**
 * @module Path
 *
 * Example usage:
 * ```js
 * import { Path } from 'socket:path'
 * ```
 */
import location from '../location.js'
import {
  resolve as resolveURL,
  parse as parseURL,
  URL,
  URLPattern
} from '../url.js'

const windowsDriveRegex = /^[a-z]:/i
const windowsDriveAndSlashesRegex = /^([a-z]:(\\|\/\/))/i
const windowsDriveInPathRegex = /^\/[a-z]:/i

function maybeURL (uri, baseURL) {
  let url = null

  try {
    baseURL = new URL(baseURL)
  } catch {}

  try {
    url = new URL(uri, baseURL)
  } catch (_) {}

  return url
}

/**
 * The path.resolve() method resolves a sequence of paths or path segments into an absolute path.
 * @param {strig} ...paths
 * @returns {string}
 * @see {@link https://nodejs.org/api/path.html#path_path_resolve_paths}
 */
export function resolve (options, ...components) {
  const { sep } = options
  let resolved = ''
  while (components.length) {
    let component = components.shift().replace(/\\/g, '/')

    if (component.length > 1) {
      component = component.replace(/\/$/g, '')
    }

    resolved = resolveURL(resolved + '/', component)

    if (resolved.length > 1) {
      resolved = resolved.replace(/\/$/g, '')
    }
  }

  if (sep === '\\') {
    resolved = resolved.replace(/\//g, sep)
  }

  if (resolved.length > 1) {
    resolved = resolved.replace(new RegExp(`\\${sep}$`, 'g'), '')
  }

  return resolved
}

/**
 * Computes current working directory for a path
 * @param {object=} [opts]
 * @param {boolean=} [opts.posix] Set to `true` to force POSIX style path
 * @return {string}
 */
export function cwd (opts) {
  let cwd = '.'

  cwd = location.pathname

  if (cwd && !cwd.endsWith('/')) {
    cwd = maybeURL('..', location)?.pathname
  }

  if (!cwd) {
    cwd = '.'
  }

  if (cwd && opts?.posix === true) {
    cwd = cwd.replace(/\\/g, '/')
  }

  return cwd
}

/**
 * Computed location origin. Defaults to `socket:///` if not available.
 * @return {string}
 */
export function origin () {
  return location.origin
}

/**
 * Computes the relative path from `from` to `to`.
 * @param {object} options
 * @param {PathComponent} from
 * @param {PathComponent} to
 * @return {string}
 */
export function relative (options, from, to) {
  const { sep } = options
  if (from === to) return ''
  from = resolve(options, from)
  to = resolve(options, to)

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

  const relative = components.output.join(sep)

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
export function join (options, ...components) {
  const { sep } = options
  const queries = []
  const resolved = []
  let protocol = null

  const isAbsolute = components[0].trim().startsWith(sep)

  while (components.length) {
    let component = String(components.shift() || '')
    const url = parseURL(component) || component

    if (url.protocol) {
      if (!protocol) {
        protocol = url.protocol
      }

      component = url.pathname
    }

    const parts = component.split(sep).filter(Boolean)
    while (parts.length) {
      queries.push(parts.shift())
    }
  }

  for (const query of queries) {
    if (query === '..' && resolved.length > 1 && resolved[0] !== '..') {
      resolved.pop()
    } else if (query !== '.') {
      if (query.startsWith(sep)) {
        resolved.push(query.slice(1))
      } else if (query.endsWith(sep)) {
        resolved.push(query.slice(0, query.length - 1))
      } else {
        resolved.push(query)
      }
    }
  }

  const joined = resolved.join(sep)

  return isAbsolute
    ? sep + joined
    : joined
}

/**
 * Computes directory name of path.
 * @param {object} options
 * @param {...PathComponent} components
 * @return {string}
 */
export function dirname (options, path) {
  if (windowsDriveInPathRegex.test(path)) {
    path = path.slice(1)
  }

  const { sep } = options
  const [drive] = path.match(windowsDriveRegex) || []

  const pathWithoutDrive = path.replace(windowsDriveAndSlashesRegex, '')
  let resolved = resolve(options, pathWithoutDrive, '..')

  if (!pathWithoutDrive.startsWith(sep)) {
    if (resolved.startsWith(sep)) {
      if (resolved === sep && !drive) {
        resolved = '.'
      } else if (pathWithoutDrive.startsWith('.')) {
        resolved = '.' + resolved
      } else {
        resolved = resolved.slice(1)
      }
    }
  }

  if (drive) {
    return `${drive}${sep}${resolved}`
  }

  return resolved
}

/**
 * Computes base name of path.
 * @param {object} options
 * @param {...PathComponent} components
 * @return {string}
 */
export function basename (options, path) {
  const { sep } = options
  const components = path.endsWith(sep)
    ? path.slice(0, -1).split(sep)
    : path.split(sep)
  return components.pop()
}

/**
 * Computes extension name of path.
 * @param {object} options
 * @param {PathComponent} path
 * @return {string}
 */
export function extname (options, path) {
  const { sep } = options
  return Path.from(path, sep).ext
}

/**
 * Computes normalized path
 * @param {object} options
 * @param {PathComponent} path
 * @return {string}
 */
export function normalize (options, path) {
  const { sep } = options
  path = String(path)
  const [drive] = path.match(windowsDriveRegex) || []
  path = path
    .replace(windowsDriveAndSlashesRegex, '')
    .replace(new RegExp(`(${sep + sep})+`, 'g'), sep)

  const url = maybeURL(path)
  const pathWithoutDrive = url?.pathname || path
  let resolved = resolve(options, pathWithoutDrive)

  if (url?.protocol) {
    if (!pathWithoutDrive.startsWith(sep) && resolved.startsWith(sep)) {
      resolved = resolved.slice(1)
      resolved = `${url?.protocol}${url?.hostname || ''}${resolved}`
    } else {
      resolved = `${url?.protocol}/${url?.hostname || ''}${resolved}`
    }
  }

  if (!pathWithoutDrive.startsWith(sep)) {
    if (resolved.startsWith(sep)) {
      if (resolved === sep && !drive) {
        resolved = '.'
      } else if (pathWithoutDrive.startsWith('.')) {
        resolved = '.' + resolved
      } else {
        resolved = resolved.slice(1)
      }
    }
  }

  const normalized = resolved
    .replace(/(\/)+/g, '/')
    .replace(/\//g, sep) + (path.endsWith(sep) ? sep : '')

  if (drive) {
    return `${drive}${sep}${normalized}`
  }

  return normalized
}

/**
 * Formats `Path` object into a string.
 * @param {object} options
 * @param {object|Path} path
 * @return {string}
 */
export function format (options, path) {
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
export function parse (path) {
  const sep = /\\/.test(path) ? '\\' : '/'
  const [drive] = path.match(windowsDriveRegex) || []
  const parsed = { root: '', dir: '', base: '', ext: '', name: '' }
  const parts = path.replace(windowsDriveRegex, '').split(sep)

  parsed.root = !parts[0] ? parts[0] || sep : ''
  parsed.dir = !parts[0] || parts[0].startsWith('.')
    ? parts.slice(0, -1).join(sep)
    : [parsed.root].concat(parts.slice(1).slice(0, -1)).join(sep)
  parsed.ext = extname({ sep }, path)
  parsed.base = basename({ sep }, path)
  parsed.name = parsed.base.replace(parsed.ext, '')

  if (!parsed.dir) parsed.dir = parsed.root
  if (drive && parsed.root) parsed.root = drive + parsed.root
  if (drive && parsed.dir) parsed.dir = drive + parsed.dir

  return parsed
}

/**
 * @typedef {(string|Path|URL|{ pathname: string }|{ url: string)} PathComponent
 */

/**
 * A container for a parsed Path.
 */
export class Path {
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

  #backSlashesDetected = false
  #leadingDot = false
  #isRelative = false
  #hasProtocol = false
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
      cwd = cwd.replace(/\\/g, '/')
      cwd = new URL(`file://${cwd.replace('file://', '')}`)
    } else if (pathname.startsWith('..')) {
      pathname = pathname.slice(2)
      cwd = 'file:///..'
    } else if (isRelative) {
      cwd = new URL('file:///.')
    } else {
      cwd = new URL(`file://${Path.cwd()}`)
    }

    if (cwd === 'socket:/') {
      cwd = Path.origin()
    }

    if (pathname.startsWith('.')) {
      this.#leadingDot = true
    }

    try {
      this.pattern = new URLPattern(pathname)
      this.#hasProtocol = Boolean(this.pattern.protocol)
    } catch {}

    this.url = new URL(pathname, cwd)

    const [drive] = (
      pathname.match(windowsDriveRegex) ||
      this.pathname.match(windowsDriveRegex) ||
      []
    )

    this.#backSlashesDetected = /\\/.test(pathname)
    this.#isRelative = isRelative
    this.#source = pathname
    this.#drive = drive ? drive.replace(/(\\|\/)/g, '') : null
  }

  get pathname () {
    let { pathname } = this.url

    if (this.#leadingDot || this.isRelative) {
      pathname = pathname.replace(/^\//g, '')
    }

    if (this.#backSlashesDetected) {
      pathname = pathname.replace(/\//g, '\\')
    }

    return pathname
  }

  get protocol () {
    return this.url.protocol
  }

  get href () {
    return this.url.href
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
    let { href } = this

    if (!this.#hasProtocol) {
      href = href.replace(regex, '')
      if (this.isRelative) {
        // eslint-disable-next-line
        href = href.replace(/^[\/|\\]/g, '')
      }
    }

    if (this.#backSlashesDetected) {
      const prefix = this.#hasProtocol ? href.slice(0, protocol.length) : ''
      href = href.slice(prefix.length)
      if (prefix.length) {
        href = prefix + '//' + href.slice(2).replace(/\//g, '\\')
      } else {
        href = href.replace(/\//g, '\\')
      }
    }

    if (!drive) {
      href = href.replace(windowsDriveInPathRegex, '')
    }

    return href
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
      if (this.value.includes('\\') || this.#backSlashesDetected) {
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

    if (this.#backSlashesDetected) {
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

    const pathname = (i > -1)
      ? this.pathname.slice(i)
      : this.pathname

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

Object.assign(Path, {
  basename,
  cwd,
  dirname,
  extname,
  format,
  join,
  normalize,
  origin,
  parse,
  relative,
  resolve
})

export default Path
