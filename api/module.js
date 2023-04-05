/* global XMLHttpRequest */
import { ModuleNotFoundError } from './errors.js'
import { ErrorEvent, Event } from './events.js'
import ipc, { Headers } from './ipc.js'
import { Stats } from './fs/stats.js'
import process from './process.js'
import console from './console.js'
import path from './path.js'

import * as exports from './module.js'
export default exports

function exists (url) {
  const { pathname } = new URL(url)
  const result = ipc.sendSync('fs.stat', {
    path: path.normalize(pathname)
  })

  if (result.err) {
    return false
  }

  return true
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
    // can throw `InvalidStateError` error
    response = request.responseText
  } catch {
    response = request.response
  }

  const headers = Headers.from(request)

  return { headers, response }
}

/**
 * @ignore
 */
function CommonJSModuleScope (
  exports,
  require,
  module,
  __filename,
  __dirname,
  process,
  global
) {
  'module code'
}

/**
 * TODO
 */
export const COMMONJS_WRAPPER = CommonJSModuleScope
  .toString()
  .split(/'module code'/)

/**
 * TODO
 */
export const MAIN_SOURCE_URL = (
  globalThis.origin === 'file://' && globalThis.location?.href
    ? globalThis.location.href
    : `file://${process.cwd() || ''}`
)

/**
 * TODO
 */
export function createRequire (sourceURL) {
  if (!sourceURL) {
    return Module.main.createRequire()
  }

  return Module.from(sourceURL).createRequire()
}

/**
 * TODO
 */
export class Module extends EventTarget {
  /**
   * TODO
   */
  static cache = Object.create(null)

  /**
   * TODO
   */
  static wrapper = COMMONJS_WRAPPER

  /**
   * TODO
   */
  static createRequire = createRequire

  /**
   * TODO
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
   * TODO
   */
  static wrap (source) {
    const [head, tail] = this.wrapper
    const body = String(source || '')
    return [head, body, tail].join('\n')
  }

  /**
   * TODO
   */
  static from (sourceURL, parent) {
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

    const url = new URL(sourceURL, parent?.id || parent)
    const id = String(url)

    if (id in this.cache) {
      return this.cache[id]
    }

    const module = new Module(id, parent, sourceURL)
    this.cache[module.id] = module
    return module
  }

  /**
   * TODO
   */
  id = null

  /**
   * TODO
   */
  path = null

  /**
   * TODO
   */
  parent = null

  /**
   * TODO
   */
  loaded = false

  /**
   * TODO
   */
  exports = {}

  /**
   * TODO
   */
  filename = null

  /**
   * TODO
   */
  children = []

  /**
   * TODO
   * @ignore
   */
  constructor (id, parent, sourceURL) {
    super()

    this.id = id || ''
    this.parent = parent || null
    this.sourceURL = sourceURL || id

    this.addEventListener('error', (event) => {
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
   * TODO
   */
  get isMain () {
    return this.id === MAIN_SOURCE_URL
  }

  /**
   * TODO
   */
  get isNamed () {
    return !this.sourceURL.startsWith('.')
  }

  /**
   * TODO
   */
  get url () {
    return String(
      this.sourceURL.startsWith('.')
        ? new URL(this.id, Module.main.sourceURL)
        : new URL(this.sourceURL, Module.main.sourceURL)
    )
  }

  /**
   * TODO
   */
  load () {
    const { url } = this
    const extname = path.extname(url)
    const prefixes = (process.env.SOCKET_MODULE_PATH_PREFIX || '').split(':')
    const queries = []

    if (!this.isNamed) {
      if (extname) {
        queries.push(url)
      } else {
        queries.push(
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
        const prefixed = path.join(prefix, this.sourceURL)
        const url = String(new URL(prefixed, Module.main.sourceURL))
        if (loadPackage(this, url)) {
          break
        }
      }
    }

    return this.loaded

    function loadPackage (module, url) {
      const result = request(path.join(url, 'package.json'))
      if (result.response) {
        try {
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
          module.dispatchEvent(new ErrorEvent('error', { error }))
        }
      }

      return module.loaded
    }

    function evaluate (module, filename, result) {
      const dirname = path.dirname(filename)
      const source = Module.wrap(result.response)

      try {
        // eslint-disable-next-line no-new-func
        const define = new Function(`return ${source}`)().bind(null)

        define(
          module.exports,
          module.require.bind(module),
          module,
          filename,
          dirname,
          process,
          globalmodule
        )

        module.id = filename
        module.path = dirname
        module.loaded = true
        module.filename = filename

        module.parent.children.push(module)
        module.dispatchEvent(new Event('load'))
        return true
      } catch (error) {
        module.dispatchEvent(new ErrorEvent('error', { error }))
        return false
      } finally {
        Object.freeze(module)
        Object.seal(module)
      }
    }
  }

  /**
   * TODO
   */
  createRequire () {
    const module = this
    const cache = {}
    Object.assign(require, { cache })
    Object.freeze(require)
    Object.seal(require)
    return require
    function require (filename) {
      return module.require(filename)
    }
  }

  /**
   * TODO
   */
  require (filename) {
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
   * TODO
   * @ignore
   */
  [Symbol.toStringTag] () {
    return 'Module'
  }
}

Object.seal(Module)
