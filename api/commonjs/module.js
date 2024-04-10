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
 * @typedef {function(string, Module, function(string): any): any} ModuleResolver
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
 *   loaders?: object,
 *   package?: Package | PackageOptions
 *   parent?: Module,
 *   state?: State
 * }} ModuleOptions
 */

/**
 * @typedef {{
 *   extensions?: object
 * }} ModuleLoadOptions
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
 * @param {typeof process} process
 * @param {object} global
 */
export function CommonJSModuleScope (
  exports,
  require,
  module,
  __filename,
  __dirname,
  process,
  global
) {
  // eslint-disable-next-line no-unused-vars
  const crypto = require('socket:crypto')
  // eslint-disable-next-line no-unused-vars
  const { Buffer } = require('socket:buffer')

  // eslint-disable-next-line no-unused-expressions
  void exports, require, module, __filename, __dirname
  // eslint-disable-next-line no-unused-expressions
  void process, console, global, crypto, Buffer

  return (function () {
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
 * @ignore
 */
export class ModuleScope {
  #module = null
  #exports = Object.create(null)

  /**
   * `ModuleScope` class constructor.
   * @param {Module} module
   */
  constructor (module) {
    this.#module = module
    Object.freeze(this)
  }

  get id () {
    return this.#module.id
  }

  get filename () {
    return this.#module.filename
  }

  get loaded () {
    return this.#module.loaded
  }

  get children () {
    return this.#module.children
  }

  get exports () {
    return this.#exports
  }

  set exports (exports) {
    this.#exports = exports
  }

  toJSON () {
    return {
      id: this.id,
      filename: this.filename,
      children: this.children,
      exports: this.exports
    }
  }
}

/**
 * An abstract base class for loading a module.
 */
export class ModuleLoader {
  /**
   * Creates a `ModuleLoader` instance from the `module` currently being loaded.
   * @param {Module} module
   * @param {ModuleLoadOptions=} [options]
   * @return {ModuleLoader}
   */
  static from (module, options = null) {
    const loader = new this(module, options)
    return loader
  }

  /**
   * Creates a new `ModuleLoader` instance from the `module` currently
   * being loaded with the `source` string to parse and load with optional
   * `ModuleLoadOptions` options.
   * @param {Module} module
   * @param {ModuleLoadOptions=} [options]
   * @return {boolean}
   */
  static load (module, options = null) {
    return this.from(module, options).load(module, options)
  }

  /**
   * @param {Module} module
   * @param {ModuleLoadOptions=} [options]
   * @return {boolean}
   */
  load (module, options = null) {
    // eslint-disable-next-line
    void module
    // eslint-disable-next-line
    void options
    return false
  }
}

/**
 * A JavaScript module loader
 */
export class JavaScriptModuleLoader extends ModuleLoader {
  /**
   * Loads the JavaScript module.
   * @param {Module} module
   * @param {ModuleLoadOptions=} [options]
   * @return {boolean}
   */
  load (module, options = null) {
    const response = module.loader.load(module.id, options)
    const compiled = Module.compile(response.text, { url: response.id })
    const __filename = module.id
    const __dirname = path.dirname(__filename)

    // eslint-disable-next-line no-useless-call
    const result = compiled.call(null,
      module.scope.exports,
      module.createRequire(options),
      module.scope,
      __filename,
      __dirname,
      process,
      globalThis
    )

    if (typeof result?.catch === 'function') {
      result.catch((error) => {
        error.module = module
        module.dispatchEvent(new ErrorEvent('error', { error }))
      })
    }

    return true
  }
}

/**
 * A JSON module loader.
 */
export class JSONModuleLoader extends ModuleLoader {
  /**
   * Loads the JSON module.
   * @param {Module} module
   * @param {ModuleLoadOptions=} [options]
   * @return {boolean}
   */
  load (module, options = null) {
    const response = module.loader.load(module.id, options)
    if (response.text) {
      module.scope.exports = JSON.parse(response.text)
    }
    return true
  }
}
/**
 * A WASM module loader

 */
export class WASMModuleLoader extends ModuleLoader {
  /**
   * Loads the WASM module.
   * @param {string}
   * @param {Module} module
   * @param {ModuleLoadOptions=} [options]
   * @return {boolean}
   */
  load (module, options = null) {
    const response = module.loader.load(module.id, {
      ...options,
      responseType: 'arraybuffer'
    })

    const instance = new WebAssembly.Instance(
      new WebAssembly.Module(response.buffer),
      options || undefined
    )

    module.scope.exports = instance.exports
    return true
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
   * Globabl module loaders
   * @type {object}
   */
  static loaders = Object.assign(Object.create(null), {
    '.js' (module, options = null) {
      return JavaScriptModuleLoader.load(module, options)
    },

    '.cjs' (module, options = null) {
      return JavaScriptModuleLoader.load(module, options)
    },

    '.json' (module, options = null) {
      return JSONModuleLoader.load(module, options)
    },

    '.wasm' (source, module, options = null) {
      return WASMModuleLoader.load(module, options)
    }
  })

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
   * Compiles given JavaScript module source.
   * @param {string} source
   * @param {{ url?: URL | string }=} [options]
   * @return {function(
   *   object,
   *   function(string): any,
   *   Module,
   *   string,
   *   string,
   *   typeof process,
   *   object
   * ): any}
   */
  static compile (source, options = null) {
    const wrapped = Module.wrap(source)
      .replace('function CommonJSModuleScope', `"Module (${options?.url ?? '<anonymous>'})"`)
    // eslint-disable-next-line
    const compiled = new Function(`
    const __commonjs_module_scope_container__ = {${wrapped}};
    return __commonjs_module_scope_container__[Object.keys(__commonjs_module_scope_container__)[0]];
    // # sourceURL=${options?.url ?? ''}
    `)()

    return compiled
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
    const module = Module.from(url, {
      package: { info: Module.main.package.info },
      ...options
    })

    return module.createRequire(options)
  }

  #id = null
  #scope = null
  #state = new State()
  #cache = Object.create(null)
  #loader = null
  #parent = null
  #package = null
  #children = []
  #resolvers = []
  #importmap = new ImportMap()
  #loaders = Object.create(null)

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

    this.#scope = new ModuleScope(this)
    this.#loader = new Loader(this.#id, options?.loader)
    this.#package = options.package instanceof Package
      ? options.package
      : this.#parent?.package ?? new Package(options.name ?? this.#id, options.package)

    this.addEventListener('error', (event) => {
      if (event.error) {
        this.#state.error = event.error
      }

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
    Object.assign(this.#loaders, Module.loaders)

    if (options.importmap) {
      this.#importmap.extend(options.importmap)
    }

    if (Array.isArray(options.resolvers)) {
      for (const resolver of options.resolvers) {
        if (typeof resolver === 'function') {
          this.#resolvers.push(resolver)
        }
      }
    }

    // includes `.browser` field mapping
    for (const key in this.package.imports) {
      const value = this.package.imports[key]
      if (value) {
        this.#resolvers.push((specifier, ctx, next) => {
          if (specifier === key) {
            return value.default ?? value.browser ?? next(specifier)
          }

          return next(specifier)
        })
      }
    }

    if (this.#parent) {
      Object.assign(this.#loaders, this.#parent.loaders)

      if (Array.isArray(options?.resolvers)) {
        for (const resolver of options.resolvers) {
          if (typeof resolver === 'function') {
            this.#resolvers.push(resolver)
          }
        }
      }

      this.#importmap.extend(this.#parent.importmap)
    }

    if (options.loaders && typeof options.loaders === 'object') {
      Object.assign(this.#loaders, options.loaders)
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
    const resolvers = Array.from(Module.resolvers).concat(this.#resolvers)
    return Array.from(new Set(resolvers))
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
   * @type {ModuleScope}
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
   * The filename of the module.
   * @type {string}
   */
  get filename () {
    return this.#id
  }

  /**
   * Known source loaders for this module keyed by file extension.
   * @type {object}
   */
  get loaders () {
    return this.#loaders
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
   * Requires a module at for a given `input` which can be a relative file,
   * named module, or an absolute URL within the context of this odule.
   * @param {string|URL} input
   * @param {RequireOptions=} [options]
   * @throws ModuleNotFoundError
   * @throws ReferenceError
   * @throws SyntaxError
   * @throws TypeError
   * @return {any}
   */
  require (url, options = null) {
    const require = this.createRequire(options)
    return require(url, options)
  }

  /**
   * Loads the module
   * @param {ModuleLoadOptions=} [options]
   * @return {boolean}
   */
  load (options = null) {
    const extension = path.extname(this.id)

    if (this.#state.loaded) {
      return true
    }

    if (typeof this.#loaders[extension] !== 'function') {
      return false
    }

    try {
      this.#state.loading = true

      if (this.#loaders[extension](this, options)) {
        if (this.#parent) {
          this.#parent.children.push(this)
        }

        this.#state.loaded = true
      }
    } catch (error) {
      error.module = this
      this.#state.error = error
      this.dispatchEvent(new ErrorEvent('error', { error }))
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

/**
 * Creates a `require` function from a given module URL.
 * @param {string|URL} url
 * @param {ModuleOptions=} [options]
 * @return {RequireFunction}
 */
export function createRequire (url, options = null) {
  return Module.createRequire(url, options)
}

builtins.module = Module
builtins.module.Module = Module

export default Module
