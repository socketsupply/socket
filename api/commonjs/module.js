/* global ErrorEvent */
/* eslint-disable no-void, no-sequences */
import { globalPaths, createRequire as createRequireImplementation } from './require.js'
import { DEFAULT_PACKAGE_PREFIX, Package } from './package.js'
import application from '../application.js'
import { Loader } from './loader.js'
import location from '../location.js'
import builtins from './builtins.js'
import process from '../process.js'
import path from '../path.js'

/**
 * @typedef {import('./require.js').RequireResolver[]} ModuleResolver
 */

/**
 * @typedef {import('./require.js').RequireFunction} RequireFunction
 */

/**
 * @typedef {import('./package.js').PackageOptions} PackageOptions
 */

/**
 * @typedef {{
 *   prefix?: string,
 *   request?: import('./loader.js').RequestOptions,
 *   builtins?: object
 * } CreateRequireOptions
 */

/**
 * @typedef {{
 *   resolvers?: ModuleResolver[],
 *   importmap?: ImportMap,
 *   loader?: Loader | object,
 *   package?: Package | PackageOptions
 *   parent?: Module,
 *   state?: State
 * }} ModuleOptions
 */

export const builtinModules = builtins

/**
 * CommonJS module scope with module scoped globals.
 * @ignore
 * @param {object} exports
 * @param {function(string): any} require
 * @param {Module} module
 * @param {string} __filename
 * @param {string} __dirname
 */
export function CommonJSModuleScope (
  exports,
  require,
  module,
  __filename,
  __dirname
) {
  // eslint-disable-next-line no-unused-vars
  const process = require('socket:process')
  // eslint-disable-next-line no-unused-vars
  const console = require('socket:console')
  // eslint-disable-next-line no-unused-vars
  const crypto = require('socket:crypto')
  // eslint-disable-next-line no-unused-vars
  const { Buffer } = require('socket:buffer')
  // eslint-disable-next-line no-unused-vars
  const global = new Proxy(globalThis, {
    get (target, key, receiver) {
      if (key === 'process') {
        return process
      }

      if (key === 'console') {
        return console
      }

      if (key === 'crypto') {
        return crypto
      }

      if (key === 'Buffer') {
        return Buffer
      }

      if (key === 'global') {
        return global
      }

      return Reflect.get(target, key, receiver)
    }
  })

  // eslint-disable-next-line no-unused-expressions
  void exports, require, module, __filename, __dirname
  // eslint-disable-next-line no-unused-expressions
  void process, console, global, crypto, Buffer

  return (async function () {
    'module code'
  })()
}

/**
 * CommonJS module scope source wrapper.
 * @type {string}
 */
export const COMMONJS_WRAPPER = CommonJSModuleScope
  .toString()
  .split(/'module code'/)

/**
 * A container for imports.
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/script/type/importmap}
 */
export class ImportMap {
  #imports = {}

  /**
   * The imports object for the importmap.
   * @type {object}
   */
  get imports () { return this.#imports }
  set imports (imports) {
    if (imports && typeof imports === 'object' && !Array.isArray(imports)) {
      this.#imports = {}
      for (const key in imports) {
        this.#imports[key] = imports[key]
      }
    }
  }

  /**
   * Extends the current imports object.
   * @param {object} imports
   * @return {ImportMap}
   */
  extend (importmap) {
    this.imports = { ...this.#imports, ...importmap?.imports ?? null }
    return this
  }
}

/**
 * A container for `Module` instance state.
 */
export class State {
  loading = false
  loaded = false
  error = null

  /**
   * `State` class constructor.
   * @ignore
   * @param {object|State=} [state]
   */
  constructor (state = null) {
    if (state && typeof state === 'object') {
      for (const key in state) {
        if (key in this && typeof state[key] === typeof this[key]) {
          this[key] = state[key]
        }
      }
    }
  }
}

/**
 * The module scope for a loaded module.
 * This is a special object that is seal, frozen, and only exposes an
 * accessor the 'exports' field.
 */
export class Scope {
  #exports = Object.create(null)

  /**
   * `Scope` class constructor.
   */
  constructor () {
    Object.freeze(this)
  }

  get exports () {
    return this.#exports
  }

  set exports (exports) {
    this.#exports = exports
  }

  toJSON () {
    return {
      exports: this.#exports
    }
  }
}

/**
 * A container for a loaded CommonJS module. All errors bubble
 * to the "main" module and global object (if possible).
 */
export class Module extends EventTarget {
  /**
   * A reference to the currently scoped module.
   * @type {Module?}
   */
  static current = null

  /**
   * A reference to the previously scoped module.
   * @type {Module?}
   */
  static previous = null

  /**
   * A cache of loaded modules
   * @type {Map<string, Module>}
   */
  static cache = new Map()

  /**
   * An array of globally available module loader resolvers.
   * @type {ModuleResolver[]}
   */
  static resolvers = []

  /**
   * Globally available 'importmap' for all loaded modules.
   * @type {ImportMap}
   * @see {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/script/type/importmap}
   */
  static importmap = new ImportMap()

  /**
   * A limited set of builtins exposed to CommonJS modules.
   * @type {object}
   */
  static builtins = builtins

  /**
   * A limited set of builtins exposed to CommonJS modules.
   * @type {object}
   */
  static builtinModules = builtinModules

  /**
   * CommonJS module scope source wrapper components.
   * @type {string[]}
   */
  static wrapper = COMMONJS_WRAPPER

  /**
   * An array of global require paths, relative to the origin.
   * @type {string[]}
   */
  static globalPaths = globalPaths

  /**
   * The main entry module, lazily created.
   * @type {Module}
   */
  static get main () {
    if (this.cache.has(location.origin)) {
      return this.cache.get(location.origin)
    }

    const main = new Module(location.origin, {
      state: new State({ loaded: true }),
      parent: null,
      package: new Package(location.origin, {
        id: location.href,
        type: 'commonjs',
        info: application.config,
        index: '',
        prefix: '',
        manifest: '',

        // eslint-disable-next-line
        name: application.config.build_name,
        // eslint-disable-next-line
        version: application.config.meta_version,
        // eslint-disable-next-line
        description: application.config.meta_description,
        exports: {
          '.': {
            default: location.pathname
          }
        }
      })
    })

    this.cache.set(location.origin, main)

    if (globalThis.window && globalThis === globalThis.window) {
      try {
        const importmapScriptElement = globalThis.document.querySelector('script[type=importmap]')
        if (importmapScriptElement && importmapScriptElement.textContent) {
          const importmap = JSON.parse(importmapScriptElement.textContent)
          Module.importmap.extend(importmap)
        }
      } catch (err) {
        globalThis.reportError(err)
      }
    }

    return main
  }

  /**
   * Wraps source in a CommonJS module scope.
   * @param {string} source
   */
  static wrap (source) {
    const [head, tail] = this.wrapper
    const body = String(source || '')
    return [head, body, tail].join('\n')
  }

  /**
   * Creates a `Module` from source URL and optionally a parent module.
   * @param {string|URL|Module} url
   * @param {ModuleOptions=} [options]
   */
  static from (url, options = null) {
    if (typeof url === 'object' && url instanceof Module) {
      return this.from(url.id, options)
    }

    if (options instanceof Module) {
      options = { parent: options }
    } else {
      options = { ...options }
    }

    if (!options.parent) {
      options.parent = Module.current
    }

    url = Loader.resolve(url, options.parent?.id)

    if (this.cache.has(url)) {
      return this.cache.get(url)
    }

    const module = new Module(url, options)
    return module
  }

  /**
   * Creates a `require` function from a given module URL.
   * @param {string|URL} url
   * @param {ModuleOptions=} [options]
   */
  static createRequire (url, options = null) {
    const module = this.from(url, {
      package: { info: Module.main.package.info },
      ...options
    })

    return module.createRequire(options)
  }

  #id = null
  #scope = new Scope()
  #state = new State()
  #cache = Object.create(null)
  #source = null
  #loader = null
  #parent = null
  #package = null
  #children = []
  #resolvers = []
  #importmap = new ImportMap()

  /**
   * `Module` class constructor.
   * @param {string|URL} url
   * @param {ModuleOptions=} [options]
   */
  constructor (url, options = null) {
    super()

    options = { ...options }

    if (options.parent && options.parent instanceof Module) {
      this.#parent = options.parent
    } else if (options.parent === undefined) {
      this.#parent = Module.main
    }

    if (this.#parent !== null) {
      this.#id = Loader.resolve(url, this.#parent.id)
      this.#cache = this.#parent.cache
    } else {
      this.#id = Loader.resolve(url)
    }

    if (options.state && options.state instanceof State) {
      this.#state = options.state
    }

    this.#loader = options?.loader instanceof Loader
      ? options.loader
      : new Loader(this.#id, options?.loader)

    this.#package = options.package instanceof Package
      ? options.package
      : this.#parent?.package ?? new Package(options.name ?? this.#id, options.package)

    this.addEventListener('error', (event) => {
      if (Module.main === this) {
        // bubble error to globalThis, if possible
        if (typeof globalThis.dispatchEvent === 'function') {
          // @ts-ignore
          globalThis.dispatchEvent(new ErrorEvent('error', event))
        }
      } else {
        // bubble errors to main module
        Module.main.dispatchEvent(new ErrorEvent('error', event))
      }
    })

    this.#importmap.extend(Module.importmap)

    if (options.importmap) {
      this.#importmap.extend(options.importmap)
    }

    this.#resolvers = Array.from(Module.resolvers)

    if (Array.isArray(options.resolvers)) {
      for (const resolver of options.resolvers) {
        if (typeof resolver === 'function') {
          this.#resolvers.push(resolver)
        }
      }
    }

    if (this.#parent) {
      if (Array.isArray(options?.resolvers)) {
        for (const resolver of options.resolvers) {
          if (typeof resolver === 'function') {
            this.#resolvers.push(resolver)
          }
        }
      }

      this.#importmap.extend(this.#parent.importmap)
    }

    Module.cache.set(this.id, this)
  }

  /**
   * A unique ID for this module.
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * A reference to the "main" module.
   * @type {Module}
   */
  get main () {
    return Module.main
  }

  /**
   * Child modules of this module.
   * @type {Module[]}
   */
  get children () {
    return this.#children
  }

  /**
   * A reference to the module cache. Possibly shared with all
   * children modules.
   * @type {object}
   */
  get cache () {
    return this.#cache
  }

  /**
   * A reference to the module package.
   * @type {Package}
   */
  get package () {
    return this.#package
  }

  /**
   * The `ImportMap` for this module.
   * @type {ImportMap}
   * @see {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/script/type/importmap}
   */
  get importmap () {
    return this.#importmap
  }

  /**
   * The module level resolvers.
   * @type {ModuleResolver[]}
   */
  get resolvers () {
    return this.#resolvers
  }

  /**
   * `true` if the module is currently loading, otherwise `false`.
   * @type {boolean}
   */
  get loading () {
    return this.#state.loading
  }

  /**
   * `true` if the module is currently loaded, otherwise `false`.
   * @type {boolean}
   */
  get loaded () {
    return this.#state.loaded
  }

  /**
   * An error associated with the module if it failed to load.
   * @type {Error?}
   */
  get error () {
    return this.#state.error
  }

  /**
   * The exports of the module
   * @type {object}
   */
  get exports () {
    return this.#scope.exports
  }

  /**
   * The scope of the module given to parsed modules.
   * @type {Scope}
   */
  get scope () {
    return this.#scope
  }

  /**
   * The origin of the loaded module.
   * @type {string}
   */
  get origin () {
    return this.#loader.origin
  }

  /**
   * The parent module for this module.
   * @type {Module?}
   */
  get parent () {
    return this.#parent
  }

  /**
   * The `Loader` for this module.
   * @type {Loader}
   */
  get loader () {
    return this.#loader
  }

  /**
   * Factory for creating a `require()` function based on a module context.
   * @param {CreateRequireOptions=} [options]
   * @return {RequireFunction}
   */
  createRequire (options = null) {
    return createRequireImplementation({
      builtins,
      // TODO(@jwerle): make the 'prefix' value configurable somehow
      prefix: DEFAULT_PACKAGE_PREFIX,
      ...options,
      module: this
    })
  }

  /**
   * Creates a `Module` from source the URL with this module as
   * the parent.
   * @param {string|URL|Module} url
   * @param {ModuleOptions=} [options]
   */
  createModule (url, options = null) {
    return Module.from(url, {
      parent: this,
      ...options
    })
  }

  /**
   * @param {object=} [options]
   * @return {boolean}
   */
  load (options = null) {
    this.#source = Module.wrap(this.loader.load(this.id).text)
    // eslint-disable-next-line
    const define = new Function(`return ${this.#source}`)()
    const __filename = this.id
    const __dirname = path.dirname(__filename)

    try {
      this.#state.loading = true
      // eslint-disable-next-line no-useless-call
      const promise = define.call(null,
        this.scope.exports,
        this.createRequire(options),
        this.scope,
        __filename,
        __dirname,
        process,
        globalThis
      )

      promise.catch((error) => {
        error.module = this
        this.dispatchEvent(new ErrorEvent('error', { error }))
      })

      if (this.parent) {
        this.parent.children.push(this)
      }

      this.#state.loaded = true
    } catch (error) {
      error.module = this
      this.#state.error = error
      throw error
    } finally {
      this.#state.loading = false
    }

    return this.#state.loaded
  }

  resolve (input) {
    return this.package.resolve(input, { load: false, origin: this.id })
  }

  /**
   * @ignore
   */
  [Symbol.toStringTag] () {
    return 'Module'
  }
}

export function createRequire (url, options = null) {
  return Module.createRequire(url, options)
}

builtins.module = Module
builtins.module.Module = Module

export default Module
