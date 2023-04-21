/* global XMLHttpRequest */
/* eslint-disable import/no-duplicates, import/first */
/**
 * @module module
 * A module for loading CommonJS modules with a `require` function in an
 * ESM context.
 */
import { ModuleNotFoundError } from './errors.js'
import { ErrorEvent, Event } from './events.js'
import { Headers } from './ipc.js'
import { Stats } from './fs/stats.js'

import * as exports from './module.js'
export default exports

// builtins
import buffer from './buffer.js'
import console from './console.js'
import dgram from './dgram.js'
import dns from './dns.js'
import events from './events.js'
import fs from './fs.js'
import gc from './gc.js'
import ipc from './ipc.js'
import os from './os.js'
import { posix as path } from './path.js'
import process from './process.js'
import stream from './stream.js'
import test from './test.js'
import util from './util.js'

/**
 * @typedef {(specifier: string, ctx: Module, next: function} => undefined} ModuleResolver
 */

function normalizePathname (pathname) {
  if (os.platform() === 'win32') {
    return path.win32.normalize(pathname.replace(/^\//, ''))
  }

  return path.normalize(pathname)
}

function exists (url) {
  const { pathname } = new URL(url)
  const result = ipc.sendSync('fs.stat', {
    path: normalizePathname(pathname)
  })

  if (result.err) {
    return false
  }

  const stats = Stats.from(result.data)

  return stats.isFile() || stats.isSymbolicLink()
}

// sync request for files
function request (url) {
  const request = new XMLHttpRequest()
  let response = null

  if (!url.startsWith('file:')) {
    url = new URL(url, 'file:///')
  }

  if (!exists(url)) {
    return {}
  }

  request.open('GET', url, false)
  request.send(null)

  try {
    // @ts-ignore
    response = request.responseText // can throw `InvalidStateError` error
  } catch {
    response = request.response
  }

  const headers = Headers.from(request)

  return { headers, response }
}

/**
 * CommonJS module scope with module scoped globals.
 * @ignore
 */
function CommonJSModuleScope (
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
  const global = new Proxy(globalThis, {
    get (target, key, receiver) {
      if (key === 'process') {
        return process
      }

      if (key === 'console') {
        return console
      }

      return Reflect.get(...arguments)
    }
  })

  return (async function () {
    'module code'
  })()
}

/**
 * A limited set of builtins exposed to CommonJS modules.
 */
export const builtins = {
  buffer,
  console,
  dgram,
  dns,
  events,
  fs,
  gc,
  ipc,
  module: exports,
  os,
  path,
  process,
  stream,
  test,
  util
}

// @ts-ignore
builtins['fs/promises'] = fs.promises
// @ts-ignore
builtins['dns/promises'] = dns.promises

/**
 * CommonJS module scope source wrapper.
 * @type {string}
 */
export const COMMONJS_WRAPPER = CommonJSModuleScope
  .toString()
  .split(/'module code'/)

/**
 * The main entry source URL.
 * @type {string}
 */
export const MAIN_SOURCE_URL = (
  globalThis.origin === 'file://' && globalThis.location?.href
    ? globalThis.location.href
    : `file://${process.cwd() || ''}`
)

/**
 * Creates a `require` function from a source URL.
 * @param {URL|string} sourceURL
 * @return {function}
 */
export function createRequire (sourceURL) {
  return Module.createRequire(sourceURL)
}

export const scope = {
  current: null,
  previous: null
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
  static get current () { return scope.current }
  static set current (module) { scope.current = module }

  /**
   * A reference to the previously scoped module.
   * @type {Module?}
   */
  static get previous () { return scope.previous }
  static set previous (module) { scope.previous = module }

  /**
   * Module cache.
   * @ignore
   */
  static cache = Object.create(null)

  /**
   * Custom module resolvers.
   * @type {Array<ModuleResolver>}
   */
  static resolvers = []

  /**
   * CommonJS module scope source wrapper.
   * @ignore
   */
  static wrapper = COMMONJS_WRAPPER

  /**
   * Creates a `require` function from a source URL.
   * @param {URL|string} sourceURL
   * @return {function}
   */
  static createRequire (sourceURL) {
    if (!sourceURL) {
      return this.main.createRequire()
    }

    return this.from(sourceURL).createRequire()
  }

  /**
   * The main entry module, lazily created.
   * @type {Module}
   */
  static get main () {
    if (MAIN_SOURCE_URL in this.cache) {
      return this.cache[MAIN_SOURCE_URL]
    }

    const main = this.cache[MAIN_SOURCE_URL] = new Module(MAIN_SOURCE_URL)
    main.filename = main.id
    main.loaded = true
    main.path = main.id
    Object.freeze(main)
    Object.seal(main)
    return main
  }

  /**
   * Wraps source in a CommonJS module scope.
   */
  static wrap (source) {
    const [head, tail] = this.wrapper
    const body = String(source || '')
    return [head, body, tail].join('\n')
  }

  /**
   * Creates a `Module` from source URL and optionally a parent module.
   * @param {string|URL|Module} [sourceURL]
   * @param {string|URL|Module} [parent]
   */
  static from (sourceURL, parent = null) {
    if (!sourceURL) {
      return Module.main
    } else if (sourceURL?.id) {
      return this.from(sourceURL.id, parent)
    } else if (sourceURL instanceof URL) {
      return this.from(String(sourceURL), parent)
    }

    if (!sourceURL.startsWith('.')) {
      parent = Module.main
    }

    // @ts-ignore
    let parentURL = parent?.id

    if (parentURL) {
      try {
        parentURL = String(new URL(parentURL, 'file:///'))
      } catch {}
    }

    const url = new URL(sourceURL, parentURL)
    const id = String(url)

    if (id in this.cache) {
      return this.cache[id]
    }

    const module = new Module(id, parent, sourceURL)
    this.cache[module.id] = module
    return module
  }

  /**
   * The module id, most likely a file name.
   * @type {string}
   */
  id = ''

  /**
   * The path to the module.
   * @type {string}
   */
  path = ''

  /**
   * The parent module, if given.
   * @type {Module?}
   */
  parent = null

  /**
   * `true` if the module did load successfully.
   * @type {boolean}
   */
  loaded = false

  /**
   * The module's exports.
   * @type {any}
   */
  exports = {}

  /**
   * The filename of the module.
   * @type {string}
   */
  filename = ''

  /**
   * Modules children to this one, as in they were required in this
   * module scope context.
   * @type {Array<Module>}
   */
  children = []

  /**
   * The original source URL to load this module.
   * @type {string}
   */
  sourceURL = ''

  /**
   * `Module` class constructor.
   * @ignore
   */
  constructor (id, parent = null, sourceURL = null) {
    super()

    this.id = id || ''
    this.parent = parent || null
    this.sourceURL = sourceURL || id

    if (!scope.current) {
      scope.current = this
    }

    if (!scope.previous && id !== MAIN_SOURCE_URL) {
      scope.previous = Module.main
    }

    this.addEventListener('error', (event) => {
      // @ts-ignore
      const { error } = event
      if (this.isMain) {
        // bubble error to globalThis, if possible
        if (typeof globalThis.dispatchEvent === 'function') {
          globalThis.dispatchEvent(new ErrorEvent('error', { error }))
        }
      } else {
        // bubble errors to main module
        Module.main.dispatchEvent(new ErrorEvent('error', { error }))
      }
    })
  }

  /**
   * `true` if the module is the main module.
   * @type {boolean}
   */
  get isMain () {
    return this.id === MAIN_SOURCE_URL
  }

  /**
   * `true` if the module was loaded by name, not file path.
   * @type {boolean}
   */
  get isNamed () {
    return !this.sourceURL?.startsWith('.')
  }

  /**
   * The `URL` for this module.
   * @type {URL}
   */
  get url () {
    return String(
      this.sourceURL?.startsWith('.')
        ? new URL(this.id, Module.main.sourceURL)
        : new URL(this.sourceURL, Module.main.sourceURL)
    )
  }

  /**
   * Loads the module, synchronously returning `true` upon success,
   * otherwise `false`.
   * @return {boolean}
   */
  load () {
    const url = this.url.replace(/\/$/, '')
    const extname = path.extname(url)
    const prefixes = (process.env.SOCKET_MODULE_PATH_PREFIX || '').split(':')
    const queries = []

    Module.previous = Module.current
    Module.current = this

    if (!this.isNamed) {
      if (extname) {
        // @ts-ignore
        queries.push(url)
      } else if (this.sourceURL.endsWith('/')) {
        queries.push(
          // @ts-ignore
          url + '/index.js'
        )
      } else {
        queries.push(
          // @ts-ignore
          url + '.js',
          url + '.json',
          url + '/index.js'
        )
      }
    }

    for (const query of queries) {
      const result = request(query)
      if (result.response) {
        evaluate(this, query, result)
        break
      }
    }

    if (!this.loaded) {
      loadPackage(this, url)
    }

    if (!this.loaded) {
      for (const prefix of prefixes) {
        const paths = [
          path.join(path.dirname(Module.previous?.id || ''), prefix, this.sourceURL)
        ]

        let dirname = path.dirname(paths[0])
        let root = path.resolve(Module.main.id)
        let max = 32 // max paths/depth

        if (path.extname(root) && !root.endsWith('/')) {
          root = path.dirname(root)
        }

        while (dirname !== root && --max > 0) {
          paths.push(path.join(dirname, prefix, this.sourceURL))
          dirname = path.dirname(dirname)
        }

        for (const prefixed of new Set(paths)) {
          const url = String(new URL(prefixed, Module.main.sourceURL))
          if (loadPackage(this, url)) {
            break
          }
        }
      }
    }

    Module.previous = this
    Module.current = null

    return this.loaded

    function loadPackage (module, url) {
      url = url.replace(/\/$/, '')
      const urls = [
        url,
        url + '.js',
        url + '.json',
        path.join(url, 'index.js'),
        path.join(url, 'index.json')
      ]

      while (urls.length) {
        const filename = urls.shift()
        const result = request(filename)
        if (result.response) {
          try {
            evaluate(module, filename, request(filename))
          } catch (error) {
            error.module = module
            module.dispatchEvent(new ErrorEvent('error', { error }))
            return false
          }
        }

        if (module.loaded) {
          return true
        }
      }

      const result = request(path.join(url, 'package.json'))
      if (result.response) {
        try {
          // @ts-ignore
          const packageJSON = JSON.parse(String(result.response))
          const filename = !packageJSON.exports
            ? path.resolve(url, packageJSON.browser || packageJSON.main)
            : (
                packageJSON.exports?.['.'] ||
                packageJSON.exports?.['./index.js'] ||
                packageJSON.exports?.['index.js']
              )

          evaluate(module, filename, request(filename))
        } catch (error) {
          error.module = module
          module.dispatchEvent(new ErrorEvent('error', { error }))
        }
      }

      return module.loaded
    }

    function evaluate (module, filename, result) {
      const dirname = path.dirname(filename)

      if (path.extname(filename) === '.json') {
        module.id = filename
        module.path = dirname
        module.filename = filename

        try {
          module.exports = JSON.parse(result.response)
        } catch (error) {
          error.module = module
          module.dispatchEvent(new ErrorEvent('error', { error }))
          return false
        } finally {
          module.loaded = true
          Object.freeze(module)
          Object.seal(module)
        }

        if (module.parent) {
          module.parent.children.push(module)
        }

        module.dispatchEvent(new Event('load'))
        return true
      }

      try {
        const source = Module.wrap(result.response)
        // eslint-disable-next-line no-new-func
        const define = new Function(`return ${source}`)()

        module.id = filename
        module.path = dirname
        module.filename = filename

        // eslint-disable-next-line no-useless-call
        const promise = define.call(null,
          module.exports,
          module.createRequire(),
          module,
          path.resolve(filename),
          path.resolve(dirname),
          process,
          globalThis
        )

        promise.catch((error) => {
          error.module = module
          module.dispatchEvent(new ErrorEvent('error', { error }))
        })

        if (module.parent) {
          module.parent.children.push(module)
        }

        module.dispatchEvent(new Event('load'))
        return true
      } catch (error) {
        error.module = module
        module.dispatchEvent(new ErrorEvent('error', { error }))
        return false
      } finally {
        module.loaded = true
        Object.freeze(module)
        Object.seal(module)
      }
    }
  }

  /**
   * Creates a require function for loaded CommonJS modules
   * child to this module.
   * @return {function(string): any}
   */
  createRequire () {
    const module = this

    Object.assign(require, { cache: Module.cache })
    Object.freeze(require)
    Object.seal(require)

    return require

    function require (filename) {
      const resolvers = Array.from(Module.resolvers)
      const result = resolve(filename, resolvers)

      if (typeof result === 'string') {
        const name = result.replace(/^(socket|node):/, '')

        if (name in builtins) {
          return builtins[name]
        }

        return module.require(name)
      }

      if (result !== undefined) {
        return result
      }

      throw new ModuleNotFoundError(
        `Cannot find module ${filename}`,
        this.children
      )
    }

    function resolve (filename, resolvers) {
      return next(filename)
      function next (specifier) {
        if (resolvers.length === 0) return specifier
        const resolver = resolvers.shift()
        return resolver(specifier, module, next)
      }
    }
  }

  /**
   * Requires a module at `filename` that will be loaded as a child
   * to this module.
   * @param {string} filename
   * @return {any}
   */
  require (filename) {
    // @ts-ignore
    const module = Module.from(filename, this)

    if (!module.load()) {
      throw new ModuleNotFoundError(
        `Cannot find module ${filename}`,
        this.children
      )
    }

    return module.exports
  }

  /**
   * @ignore
   */
  [Symbol.toStringTag] () {
    return 'Module'
  }
}

// builtins should never be overloaded through this object, instead
// a custom resolver should be used
Object.freeze(Object.seal(builtins))
// prevent misuse of the `Module` class
Object.freeze(Object.seal(Module))
