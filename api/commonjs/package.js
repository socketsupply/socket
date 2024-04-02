/**
 * @module CommonJS.Package
 */
import { ModuleNotFoundError } from '../errors.js'
import { Loader } from './loader.js'
import location from '../location.js'
import path from '../path.js'

/**
 * `true` if in a worker scope.
 * @type {boolean}
 * @ignore
 */
const isWorkerScope = globalThis.self === globalThis && !globalThis.window

/**
 * @typedef {{
 *   manifest?: string,
 *   index?: string,
 *   description?: string,
 *   version?: string,
 *   license?: string,
 *   exports?: object,
 *   type?: 'commonjs' | 'module',
 *   info?: object,
 *   origin?: string,
 *   dependencies?: Dependencies | object | Map
 * }} PackageOptions
 */

/**
 * @typedef {import('./loader.js').RequestOptions & {
 *   type?: 'commonjs' | 'module'
 *   prefix?: string
 * }} PackageLoadOptions
 */

/**
 * {import('./loader.js').RequestOptions & {
 *   load?: boolean,
 *   type?: 'commonjs' | 'module',
 *   browser?: boolean,
 *   children?: string[]
 *   extensions?: string[] | Set<string>
 * }} PackageResolveOptions
 */

/**
 * @typedef {{
 *   organization: string | null,
 *   name: string,
 *   version: string | null,
 *   pathname: string,
 *   url: URL,
 *   isRelative: boolean,
 *   hasManifest: boolean
 * }} ParsedPackageName
/**
 * The default package index file such as 'index.js'
 * @type {string}
 */
export const DEFAULT_PACKAGE_INDEX = 'index.js'

/**
 * The default package manifest file name such as 'package.json'
 * @type {string}
 */
export const DEFAULT_PACKAGE_MANIFEST_FILE_NAME = 'package.json'

/**
 * The default package path prefix such as 'node_modules/'
 * @type {string}
 */
export const DEFAULT_PACKAGE_PREFIX = 'node_modules/'

/**
 * The default package version, when one is not provided
 * @type {string}
 */
export const DEFAULT_PACKAGE_VERSION = '0.0.1'

/**
 * The default license for a package'
 * @type {string}
 */
export const DEFAULT_LICENSE = 'Unlicensed'

/**
 * A container for a package name that includes a package organization identifier,
 * its fully qualified name, or for relative package names, its pathname
 */
export class Name {
  /**
   * Parses a package name input resolving the actual module name, including an
   * organization name given. If a path includes a manifest file
   * ('package.json'), then the directory containing that file is considered a
   * valid package and it will be included in the returned value. If a relative
   * path is given, then the path is returned if it is a valid pathname. This
   * function returns `null` for bad input.
   * @param {string|URL} input
   * @param {{ origin?: string | URL, manifest?: string }=} [options]
   * @return {ParsedPackageName?}
   */
  static parse (input, options = null) {
    if (typeof input === 'string') {
      input = input.trim()
    }

    if (!input) {
      return null
    }

    const origin = options?.origin ?? location.origin
    const manifest = options?.manifest ?? DEFAULT_PACKAGE_MANIFEST_FILE_NAME
    // relative or absolute path given, which is still relative to the origin
    const isRelative = (
      typeof input === 'string' &&
      (input.startsWith('.') || input.startsWith('/'))
    )

    let hasManifest = false
    let url = null

    // URL already given, ignore the origin
    // @ts-ignore
    if (URL.canParse(input) || input instanceof URL) {
      url = new URL(input)
    } else {
      url = new URL(input, origin)
    }

    // invalid input if a URL was unable to be determined
    if (!url) {
      return null
    }

    let pathname = url.pathname.replace(new URL(origin).pathname, '')

    // manifest was given in name, just use the directory name
    if (pathname.endsWith(`/${manifest}`)) {
      hasManifest = true
      pathname = pathname.split('/').slice(0, -1).join('/')
    }

    // name included organization
    if (pathname.startsWith('@')) {
      const components = pathname.split('/')
      const organization = components[0]
      let [name, version = null] = components[1].split('@')
      pathname = [organization || '', name, ...components.slice(2)].filter(Boolean).join('/')
      // manifest was given, this could be a nested package
      if (hasManifest) {
        name = [organization, name].filter(Boolean).concat(components.slice(2)).join('/')
        return {
          name,
          version,
          organization,
          pathname,
          url,
          isRelative,
          hasManifest
        }
      }

      // only a organization was given, return `null`
      if (components.length === 1) {
        return null
      }

      name = `${organization}/${name}`

      // only `@<org>/package>` was given
      if (components.length === 2) {
        return {
          name,
          version,
          organization,
          pathname,
          url,
          isRelative,
          hasManifest
        }
      }

      // `@<org>/<package>/...` was given
      return {
        name,
        version,
        organization,
        pathname,
        url,
        isRelative,
        hasManifest
      }
    }

    // a valid relative path was given, just return it normalized
    if (isRelative) {
      if (input.startsWith('/')) {
        pathname = `/${pathname}`
      } else {
        pathname = `./${pathname}`
      }

      return {
        organization: null,
        version: null,
        name: pathname,
        pathname,
        url,
        isRelative,
        hasManifest
      }
    }

    // at this point, a named module was given
    const components = pathname.split('/')
    const [name, version = null] = components[0].split('@')
    pathname = [name, ...components.slice(1)].filter(Boolean).join('/')

    // manifest was given, this could be a nested package
    if (hasManifest) {
      return {
        organization: null,
        name: pathname,
        pathname,
        url,
        isRelative,
        hasManifest
      }
    }

    return {
      organization: null,
      name,
      version,
      pathname,
      url,
      isRelative,
      hasManifest
    }
  }

  /**
   * Returns `true` if the given `input` can be parsed by `Name.parse` or given
   * as input to the `Name` class constructor.
   * @param {string|URL} input
   * @param {{ origin?: string | URL, manifest?: string }=} [options]
   * @return {boolean}
   */
  static canParse (input, options = null) {
    const origin = options?.origin ?? location.origin

    if (typeof input === 'string') {
      input = input.trim()
    }

    if (!input) {
      return null
    }

    // URL already given, ignore the origin
    // @ts-ignore
    return input instanceof URL || URL.canParse(input, origin)
  }

  /**
   * Creates a new `Name` from input.
   * @param {string|URL} input
   * @param {{ origin?: string | URL, manifest?: string }=} [options]
   * @return {Name}
   */
  static from (input, options = null) {
    if (!Name.canParse(input, options)) {
      throw new TypeError(
        `Cannot create new 'Name'. Invalid 'input' given. Received: ${input}`
      )
    }

    return new Name(Name.parse(input, options))
  }

  #name = null
  #origin = null
  #version = null
  #pathname = null
  #organization = null

  #isRelative = false

  /**
   * `Name` class constructor.
   * @param {string|URL|NameOptions|Name} name
   * @param {{ origin?: string | URL, manifest?: string }=} [options]
   * @throws TypeError
   */
  constructor (name, options = null) {
    /** @type {ParsedPackageName?} */
    let parsed = null
    if (typeof name === 'string' || name instanceof URL) {
      parsed = Name.parse(name, options)
    } else if (name && typeof name === 'object') {
      parsed = {
        organization: name.organization || null,
        name: name.name || null,
        pathname: name.pathname || null,
        version: name.version || null,
        // @ts-ignore
        url: name.url instanceof URL || URL.canParse(name.url)
          ? new URL(name.url)
          : null,
        isRelative: name.isRelative || false,
        hasManifest: name.hasManifest || false
      }
    }

    if (parsed === null) {
      throw new TypeError(`Invalid 'name' given. Received: ${name}`)
    }

    this.#name = parsed.name
    this.#origin = parsed.url?.origin ?? location.origin
    this.#version = parsed.version ?? null
    this.#pathname = parsed.pathname
    this.#organization = parsed.organization

    this.#isRelative = parsed.isRelative
  }

  /**
   * The id of this package name.
   * @type {string}
   */
  get id () {
    return this.#pathname
  }

  /**
   * The actual package name.
   * @type {string}
   */
  get name () { return this.#name }

  /**
   * The origin of the package, if available.
   * @type {string?}
   */
  get origin () { return this.#origin }

  /**
   * The package version if available.
   * @type {string}
   */
  get version () { return this.#version }

  /**
   * The actual package pathname.
   * @type {string}
   */
  get pathname () { return this.#pathname }

  /**
   * The organization name. This value may be `null`.
   * @type {string?}
   */
  get organization () { return this.#organization }

  /**
   * `true` if the package name was relative, otherwise `false`.
   * @type {boolean}
   */
  get isRelative () { return this.#isRelative }

  /**
   * Converts this package name to a string.
   * @ignore
   * @return {string}
   */
  toString () {
    return this.id
  }

  /**
   * Converts this `Name` instance to JSON.
   * @ignore
   * @return {object}
   */
  toJSON () {
    return {
      name: this.name,
      origin: this.origin,
      version: this.version,
      pathname: this.pathname,
      organization: this.organization
    }
  }
}

/**
 * A container for package dependencies that map a package name to a `Package` instance.
 */
export class Dependencies {
  #map = new Map()
  #origin = null
  #package = null

  constructor (parent, options = null) {
    this.#package = parent
    this.#origin = options?.origin ?? parent?.origin
  }

  get map () {
    return this.#map
  }

  get origin () {
    return this.#origin ?? null
  }

  add (name, info = null) {
    if (info instanceof Package) {
      this.#map.set(name, info)
    } else {
      this.#map.set(name, new Package(name, {
        loader: this.#package.loader,
        origin: this.#origin,
        info
      }))
    }

    this.#map.get(name).load({ force: true })
  }

  get (name) {
    return this.#map.get(name)
  }

  entries () {
    return this.#map.entries()
  }

  keys () {
    return this.#map.keys()
  }

  values () {
    return this.#map.values()
  }

  load (options = null) {
    for (const dependency of this.values()) {
      dependency.load(options)
    }
  }

  [Symbol.iterator] () {
    return this.#map.entries()
  }
}

/**
 * A container for CommonJS module metadata, often in a `package.json` file.
 */
export class Package {
  /**
   * A high level class for a package name.
   * @type {typeof Name}
   */
  static Name = Name

  /**
   * A high level container for package dependencies.
   * @type {typeof Dependencies}
   */
  static Dependencies = Dependencies

  #id = null
  #name = null
  #type = 'commonjs'
  #license = null
  #version = null
  #description = null
  #dependencies = null

  #info = null
  #loader = null

  #exports = {
    '.': {
      require: './index.js',
      import: './index.js',
      default: './index.js'
    }
  }

  /**
   * `Package` class constructor.
   * @param {string|URL|NameOptions|Name} name
   * @param {PackageOptions=} [options]
   */
  constructor (name, options = null) {
    options = /** @type {PackageOptions} */ ({ ...options })

    if (typeof name !== 'string' && !(name instanceof Name) && !(name instanceof URL)) {
      throw new TypeError(`Expecting 'name' to be a string or URL. Received: ${name}`)
    }

    // the module loader
    this.#loader = options.loader instanceof Loader
      ? options.loader
      : new Loader(options.loader)

    this.#id = options.id ?? null
    this.#name = Name.from(name, {
      origin: options.origin ?? options.loader?.origin ?? this.#loader.origin ?? null,
      manifest: options.manifest ?? DEFAULT_PACKAGE_MANIFEST_FILE_NAME
    })

    // early meta data
    this.#info = options.info ?? null
    this.#exports = options.exports ?? this.#exports
    this.#license = options.license ?? DEFAULT_LICENSE
    this.#version = options.version ?? this.#name.version ?? DEFAULT_PACKAGE_VERSION
    this.#description = options.description ?? ''
    this.#dependencies = new Dependencies(this)

    if (options.dependencies && typeof options.dependencies === 'object') {
      if (options.dependencies instanceof Dependencies || typeof options.dependencies.entries === 'function') {
        for (const [key, value] of options.dependencies.entries()) {
          this.#dependencies.add(key, value)
        }
      } else {
        for (const key in options.dependencies) {
          const value = options.dependencies[key]
          this.#dependencies.add(key, value)
        }
      }
    }

    if (!this.#exports || typeof this.#exports !== 'object') {
      this.#exports = { '.': null }
    }

    if (!this.#exports['.']) {
      this.#exports = {
        '.': {
          require: options.index ?? DEFAULT_PACKAGE_INDEX,
          import: options.index ?? DEFAULT_PACKAGE_INDEX,
          default: options.index ?? DEFAULT_PACKAGE_INDEX
        }
      }
    }
  }

  /**
   * The unique ID of this `Package`, which is the absolute
   * URL of the directory that contains its manifest file.
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * The absolute URL to the package manifest file
   * @type {string}
   */
  get url () {
    return new URL(this.#id).href
  }

  /**
   * A loader for this package, if available. This value may be `null`.
   * @type {Loader}
   */
  get loader () {
    return this.#loader
  }

  /**
   * The name of the package.
   * @type {string}
   */
  get name () {
    return this.#name?.id ?? ''
  }

  /**
   * The description of the package.
   * @type {string}
   */
  get description () {
    return this.#description ?? ''
  }

  /**
   * The organization of the package. This value may be `null`.
   * @type {string?}
   */
  get organization () {
    return this.#name.organization ?? null
  }

  /**
   * The license of the package.
   * @type {string}
   */
  get license () {
    return this.#license ?? DEFAULT_LICENSE
  }

  /**
   * The version of the package.
   * @type {string}
   */
  get version () {
    return this.#version ?? DEFAULT_PACKAGE_VERSION
  }

  /**
   * The origin for this package.
   * @type {string}
   */
  get origin () {
    return this.loader.origin
  }

  /**
   * The exports mappings for the package
   * @type {object}
   */
  get exports () {
    return this.#exports
  }

  /**
   * The package type.
   * @type {'commonjs'|'module'}
   */
  get type () {
    return this.#type
  }

  /**
   * The raw package metadata object.
   * @type {object?}
   */
  get info () {
    return this.#info ?? null
  }

  /**
   * @type {Dependencies}
   */
  get dependencies () {
    return this.#dependencies
  }

  /**
   * An alias for `entry`
   * @type {string?}
   */
  get main () {
    return this.entry
  }

  /**
   * The entry to the package
   * @type {string?}
   */
  get entry () {
    let entry = null

    if (this.type === 'commonjs') {
      entry = this.#exports['.'].require
    } else if (this.type === 'module') {
      entry = this.#exports['.'].import
    }

    if (!entry && this.#exports['.'].default) {
      entry = this.#exports['.'].default
    }

    if (isWorkerScope) {
      if (!entry && this.#exports['.'].entry) {
        entry = this.#exports['.'].entry
      }
    }

    if (!entry && this.#exports['.'].browser) {
      entry = this.#exports['.'].browser
    }

    if (entry) {
      if (!entry.startsWith('./')) {
        entry = `./${entry}`
      } else if (!entry.startsWith('.')) {
        entry = `.${entry}`
      }

      return new URL(entry, this.id).href
    }

    return null
  }

  /**
   * Load the package information at an optional `origin` with
   * optional request `options`.
   * @param {PackageLoadOptions=} [options]
   * @throws SyntaxError
   * @return {boolean}
   */
  load (origin = null, options = null) {
    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = options.origin
    }

    if (options?.force !== true && this.#info) {
      return true
    }

    if (!origin) {
      origin = this.origin
    }

    const prefix = options?.prefix ?? ''
    const manifest = options?.manifest ?? DEFAULT_PACKAGE_MANIFEST_FILE_NAME
    const pathname = `${prefix}${this.name}/${manifest}`
    const response = this.loader.load(pathname, origin, options)

    if (!response.text) {
      return false
    }

    const info = JSON.parse(response.text)

    if (!info || typeof info !== 'object') {
      return false
    }

    const type = options?.type ?? info.type ?? this.#type

    this.#info = info
    this.#type = type

    this.#id = new URL('./', response.id).href
    this.#name = Name.from(info.name, { origin })
    this.#license = info.license ?? 'Unlicensed'
    this.#version = info.version
    this.#description = info.description

    if (info.dependencies && typeof info.dependencies === 'object') {
      for (const name in info.dependencies) {
        const version = info.dependencies[name]
        if (typeof version === 'string') {
          this.#dependencies.add(name, {
            version
          })
        }
      }
    }

    if (info.main) {
      this.#exports['.'].require = info.main
      this.#exports['.'].import = info.main
    }

    if (info.module) {
      this.#exports['.'].import = info.module
    }

    if (typeof info.exports === 'string') {
      if (type === 'commonjs') {
        this.#exports['.'].require = info.exports
      } else if (type === 'module') {
        this.#exports['.'].import = info.exports
      }
    }

    if (info.exports && typeof info.exports === 'object') {
      for (const key in info.exports) {
        const exports = info.exports[key]
        if (!exports) {
          continue
        }

        if (typeof exports === 'string') {
          this.#exports[key] = {}
          if (this.#type === 'commonjs') {
            this.#exports[key].require = exports
          } else if (this.#type === 'module') {
            this.#exports[key].import = exports
          }
        } else if (typeof exports === 'object') {
          this.#exports[key] = exports
        }
      }
    }

    for (const key in this.#exports) {
      const exports = this.#exports[key]
      if (Array.isArray(exports)) {
        for (let i = 0; i < exports.length; ++i) {
          const value = exports[i]
          if (typeof value === 'string') {
            if (value.startsWith('/')) {
              exports[i] = `.${value}`
            } else if (!value.startsWith('.')) {
              exports[i] = `./${value}`
            }
          }
        }
      } else {
        for (const condition in exports) {
          const value = exports[condition]
          if (Array.isArray(value)) {
            for (let i = 0; i < value.length; ++i) {
              if (typeof value[i] === 'string') {
                if (value[i].startsWith('/')) {
                  value[i] = `.${value[i]}`
                } else if (!value[i].startsWith('.')) {
                  value[i] = `./${value[i]}`
                }
              }
            }
          } else if (typeof value === 'string') {
            if (value.startsWith('/')) {
              exports[condition] = `.${value}`
            } else if (!value.startsWith('.')) {
              exports[condition] = `./${value}`
            }
          }
        }
      }
    }

    return true
  }

  /**
   * Resolve a file's `pathname` within the package.
   * @param {string|URL} pathname
   * @param {PackageResolveOptions=} [options]
   * @return {string}
   */
  resolve (pathname, options = null) {
    if (options?.load !== false) {
      this.load(options)
    }

    const { info } = this
    const manifest = options?.manifest ?? DEFAULT_PACKAGE_MANIFEST_FILE_NAME
    const extname = path.extname(pathname)
    const type = options?.type ?? this.type

    let origin = this.id

    // an absolute URL was given, just try to resolve it
    // @ts-ignore
    if (pathname instanceof URL || URL.canParse(pathname)) {
      const url = new URL(pathname)
      const response = this.loader.status(url.href, options)

      if (response.ok) {
        return interpolateBrowserResolution(response.id)
      }

      pathname = url.pathname
      origin = url.origin
    }

    if (pathname === '.') {
      pathname = './'
    } else if (pathname.startsWith('/')) {
      pathname = `.${pathname}`
    } else if (!pathname.startsWith('.')) {
      pathname = `./${pathname}`
    }

    if (options?.origin) {
      origin = options.origin
    } else if (!origin) {
      origin = new URL(this.name, this.origin).href
      if (!origin.endsWith('/')) {
        origin += '/'
      }
    }

    // if the pathname ends with the manifest file ('package.json'), then
    // construct a new `Package` with the this packages loader and resolve it
    if (pathname.endsWith(`/${manifest}`)) {
      const url = new URL(`${this.name}/${pathname}`, origin)
      const childPackage = new Package(url, {
        loader: this.loader,
        origin
      })

      // if it loaded, then just return the URL
      if (childPackage.load()) {
        return url.href
      }
    }

    const extensions = extname !== '' && this.loader.extensions.has(extname)
      ? new Set([extname])
      : new Set(Array
        .from(options?.extensions ?? [])
        .concat('')
        .concat(Array.from(this.loader.extensions))
        .filter((e) => typeof e === 'string')
      )

    if (pathname.endsWith('/')) {
      pathname += 'index.js'
    }

    for (const extension of extensions) {
      for (const key in this.#exports) {
        const exports = this.#exports[key]
        const query = pathname !== '.' && pathname !== './'
          ? pathname + extension
          : pathname

        if (
          key === query ||
          key === pathname.replace(extname, '') ||
          (pathname === './' && key === '.') ||
          (pathname === './index' && key === '.') ||
          (pathname === './index.js' && key === '.')
        ) {
          if (Array.isArray(exports)) {
            for (const filename of exports) {
              if (typeof filename === 'string') {
                const response = this.loader.load(filename, origin, options)
                if (response.ok) {
                  return interpolateBrowserResolution(response.id)
                }
              }
            }
          } else {
            let filename = null
            if (isWorkerScope && exports.worker) {
              filename = exports.worker
            } if (type === 'commonjs' && exports.require) {
              filename = exports.require
            } else if (type === 'module' && exports.import) {
              filename = exports.import
            } else if (exports.browser) {
              filename = exports.browser
            } else if (exports.default) {
              filename = exports.default
            } else {
              filename = (
                exports.require ||
                exports.import ||
                exports.browser ||
                exports.default
              )
            }

            if (filename) {
              const response = this.loader.load(filename, origin, options)
              if (response.ok) {
                return interpolateBrowserResolution(response.id)
              }
            }
          }
        }
      }

      if (!extname || !this.loader.extensions.has(extname)) {
        let response = this.loader.load(pathname + extension, origin, options)
        if (response.ok) {
          return interpolateBrowserResolution(response.id)
        }

        response = this.loader.load(`${pathname}/index${extension}`, origin, options)
        if (response.ok) {
          return interpolateBrowserResolution(response.id)
        }
      }
    }

    const response = this.loader.load(pathname, origin, options)

    if (response.ok) {
      return interpolateBrowserResolution(response.id)
    }

    // try to load 'package.json'
    const url = new URL(`${this.name}/${pathname}/${manifest}`, origin)
    const childPackage = new Package(url, {
      loader: this.loader,
      origin
    })

    // if it loaded, then return the package entry
    if (childPackage.load()) {
      return childPackage.entry
    }

    throw new ModuleNotFoundError(
      `Cannot find module '${pathname}'`,
      options?.children?.map?.((mod) => mod.id)
    )

    function interpolateBrowserResolution (id) {
      if (!info || options?.browser === false) {
        return id
      }

      const url = new URL(id)
      const prefix = new URL('.', origin).href
      const pathname = `./${url.href.replace(prefix, '')}`

      if (info.browser && typeof info.browser === 'object') {
        for (const key in info.browser) {
          const value = info.browser[key]
          const filename = !key.startsWith('./') ? `./${key}` : key
          if (filename === pathname) {
            return new URL(value, prefix).href
          }
        }
      }

      return id
    }
  }
}

export default Package
