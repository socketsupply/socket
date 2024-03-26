import { ModuleNotFoundError } from '../errors.js'
import { Loader } from './loader.js'
import location from '../location.js'
import path from '../path.js'

/**
 * @typedef {{
 *   prefix?: string,
 *   manifest?: string,
 *   index?: string,
 *   description?: string,
 *   version?: string,
 *   license?: string,
 *   exports?: object,
 *   type?: 'commonjs' | 'module',
 *   info?: object
 * }} PackageOptions
 */

/**
 * @typedef {import('./loader.js').RequestOptions & {
 *   type?: 'commonjs' | 'module'
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
 * A container for CommonJS module metadata, often in a `package.json` file.
 */
export class Package {
  /**
   * @param {string} input
   * @param {PackageOptions&PackageLoadOptions} [options]
   * @return {Package?}
   */
  static find (input, options = null) {
    const url = URL.canParse(input)
      ? new URL(input)
      : new URL(input, options?.origin ?? location.origin)

    const components = url.pathname.split('/')

    if (path.extname(url.pathname)) {
      components.pop()
    }

    while (components.length) {
      const name = components.join('/')

      if (!name) {
        return null
      }

      const manifest = new Package(name, {
        ...options,
        prefix: '',
        loader: new Loader(
          new URL(components.join('/'), url.origin),
          options?.loader
        )
      })

      if (path.extname(manifest.name)) {
        continue
      }

      if (manifest.load(options)) {
        return manifest
      }

      components.pop()
    }

    return null
  }

  #info = null
  #index = null
  #prefix = null
  #manifest = null
  #loader = null

  #id = null
  #name = null
  #type = 'commonjs'
  #exports = {}
  #license = null
  #version = null
  #description = null

  /**
   * `Package` class constructor.
   * @param {string} name
   * @param {PackageOptions} [options]
   */
  constructor (name, options = null) {
    options = { ...options }

    if (!name || typeof name !== 'string') {
      throw new TypeError(`Expecting 'name' to be a string. Received: ${name}`)
    }

    // early meta data
    this.#id = options.id ?? null
    this.#name = name
    this.#info = options.info ?? null
    this.#exports = options.exports ?? {}
    this.#license = options.license ?? DEFAULT_LICENSE
    this.#version = options.version ?? DEFAULT_PACKAGE_VERSION
    this.#description = options.description ?? ''

    // request paths
    this.#index = options.index ?? DEFAULT_PACKAGE_INDEX
    this.#prefix = options.prefix ?? DEFAULT_PACKAGE_PREFIX
    this.#manifest = options.manifest ?? DEFAULT_PACKAGE_MANIFEST_FILE_NAME

    // the module loader
    this.#loader = new Loader(options.loader)
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
    return new URL(this.#manifest, this.#id).href
  }

  /**
   * The package module path prefix.
   * @type {string}
   */
  get prefix () {
    return this.#prefix
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
    return this.#name ?? ''
  }

  /**
   * The description of the package.
   * @type {string}
   */
  get description () {
    return this.#description ?? ''
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
    return new URL(this.#prefix, this.loader.origin).href
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
   * The entry to the package
   * @type {string?}
   */
  get entry () {
    let entry = null
    if (this.type === 'commonjs') {
      entry = this.#exports['.'].require
    } else if (this.type === 'module') {
      entry = this.#exports['.'].import
    } else if (this.#exports['.'].default) {
      entry = this.#exports['.'].default
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
   * @param {string|PackageLoadOptions=} [origin]
   * @param {PackageLoadOptions=} [options]
   * @throws SyntaxError
   * @return {boolean}
   */
  load (origin, options) {
    if (this.#info) {
      return true
    }

    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = options.origin
    }

    if (!origin) {
      origin = this.origin.replace(this.#prefix, '')
    }

    const pathname = `${this.name}/${this.#manifest}`
    const response = this.loader.load(pathname, origin, options)

    if (!response.text) {
      return false
    }

    const info = JSON.parse(response.text)
    const type = options?.type ?? info.type ?? this.type

    this.#info = info
    this.#exports['.'] = {}

    this.#id = new URL('./', response.id).href
    this.#name = info.name
    this.#type = info.type ?? this.#type
    this.#license = info.license ?? 'Unlicensed'
    this.#version = info.version
    this.#description = info.description

    if (info.main) {
      if (type === 'commonjs') {
        this.#exports['.'].require = info.main
      } else if (type === 'module') {
        this.#exports['.'].import = info.main
      }
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
            this.#exports[key].require = info.exports
          } else if (this.#type === 'module') {
            this.#exports[key].import = info.exports
          }
        } else if (typeof exports === 'object') {
          this.#exports[key] = exports
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
    if (options?.load !== false && !this.load(options)) {
      throw new ModuleNotFoundError(
        `Cannnot find module '${pathname}`,
        options?.children?.map?.((mod) => mod.id)
      )
    }

    const { info } = this
    const origin = options?.origin || this.id || this.origin
    const type = options?.type ?? this.type

    if (pathname instanceof URL) {
      pathname = pathname.pathname
    }

    if (pathname === '.') {
      pathname = './'
    } else if (pathname.startsWith('/')) {
      pathname = `.${pathname}`
    } else if (!pathname.startsWith('./')) {
      pathname = `./${pathname}`
    }

    if (pathname.endsWith('/')) {
      pathname += 'index.js'
    }

    const extname = path.extname(pathname)
    const extensions = extname !== ''
      ? new Set([extname])
      : new Set(Array
        .from(options?.extensions ?? [])
        .concat(Array.from(this.loader.extensions))
        .filter((e) => typeof e === 'string' && e.length > 0)
      )

    for (const extension of extensions) {
      for (const key in this.#exports) {
        const exports = this.#exports[key]
        const query = pathname !== '.' && pathname !== './'
          ? pathname + extension
          : pathname

        if ((pathname === './' && key === '.') || key === query) {
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
            if (type === 'commonjs' && exports.require) {
              filename = exports.require
            } else if (type === 'module' && exports.import) {
              filename = exports.import
            } else if (exports.default) {
              filename = exports.default
            } else {
              filename = exports.require || exports.import || exports.default
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

      if (!extname) {
        let response = null
        response = this.loader.load(pathname + extension, origin, options)

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

    throw new ModuleNotFoundError(
      `Cannnot find module '${pathname}`,
      options?.children?.map?.((mod) => mod.id)
    )

    function interpolateBrowserResolution (id) {
      if (options?.browser === false) {
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
