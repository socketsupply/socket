/* global XMLHttpRequest */
/* eslint-disable import/no-duplicates, import/first, no-void, no-sequences */
/**
 * @module module
 * A module for loading CommonJS modules with a `require` function in an
 * ESM context.
 */
import { ModuleNotFoundError } from './errors.js'
import { ErrorEvent, Event } from './events.js'
import { Headers } from './ipc.js'
import { URL } from './url.js'

// builtins
// eslint-disable-next-line
import async_context from './async_context.js'
// eslint-disable-next-line
import async_hooks from './async_hooks.js'
import application from './application.js'
import assert from './assert.js'
import buffer from './buffer.js'
import console from './console.js'
import constants from './constants.js'
import crypto from './crypto.js'
import dgram from './dgram.js'
import dns from './dns.js'
import events from './events.js'
import extension from './extension.js'
import fs from './fs.js'
import gc from './gc.js'
import http from './http.js'
import https from './https.js'
import ipc from './ipc.js'
import language from './language.js'
import mime from './mime.js'
import os from './os.js'
import { posix as path } from './path.js'
import process from './process.js'
import querystring from './querystring.js'
import stream from './stream.js'
// eslint-disable-next-line
import string_decoder from './string_decoder.js'
import test from './test.js'
import timers from './timers.js'
import url from './url.js'
import util from './util.js'
import vm from './vm.js'
import window from './window.js'
// eslint-disable-next-line
import worker_threads from './worker_threads.js'

/**
 * @typedef {function(string, Module, function): undefined} ModuleResolver
 */

const cache = new Map()

class ModuleRequest {
  id = null
  url = null

  static load (pathname, parent) {
    const request = new this(pathname, parent)
    return request.load()
  }

  static verifyRequestContentLocation (request) {
    const headers = Headers.from(request)
    const contentLocation = headers.get('content-location')
    // if a content location was given, check extension name
    if (URL.canParse(contentLocation, globalThis.location.href)) {
      const url = new URL(contentLocation, globalThis.location.href)
      if (!/js|json|mjs|cjs|jsx|ts|tsx/.test(path.extname(url.pathname))) {
        return false
      }
    }

    return true
  }

  constructor (pathname, parent) {
    const origin = globalThis.location.origin.startsWith('blob:')
      ? new URL(new URL(globalThis.location.href).pathname).origin
      : globalThis.location.origin

    this.url = new URL(pathname, parent || origin || '/')
    this.id = this.url.toString()
  }

  status () {
    const { id } = this
    const request = new XMLHttpRequest()
    request.open('HEAD', id, false)
    request.send(null)

    if (!ModuleRequest.verifyRequestContentLocation(request)) {
      return 404
    }

    return request.status
  }

  load () {
    const { id } = this

    if (cache.has(id)) {
      return cache.get(id)
    }

    if (this.status() >= 400) {
      return new ModuleResponse(this)
    }

    const request = new XMLHttpRequest()

    request.open('GET', id, false)
    request.send(null)

    if (!ModuleRequest.verifyRequestContentLocation(request)) {
      return null
    }

    const headers = Headers.from(request)
    let responseText = null

    try {
      // @ts-ignore
      responseText = request.responseText // can throw `InvalidStateError` error
    } catch {
      responseText = request.response
    }

    const response = new ModuleResponse(this, headers, responseText)

    if (request.status < 400) {
      cache.set(id, response)
    }

    return response
  }
}

class ModuleResponse {
  request = null
  headers = null
  data = null

  constructor (request, headers, data) {
    this.request = request
    this.headers = headers ?? null
    this.data = data ?? null
  }
}

// sync request for files
function request (pathname, parent) {
  return ModuleRequest.load(pathname, parent)
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

      if (key === 'module') {
        return Module.main
      }

      if (key === 'exports') {
        return Module.main.exports
      }

      if (key === 'require') {
        return Module.main.require
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
 * A limited set of builtins exposed to CommonJS modules.
 */
export const builtins = {
  // eslint-disable-next-line
  async_context,
  // eslint-disable-next-line
  async_hooks,
  application,
  assert,
  buffer,
  console,
  constants,
  child_process: {},
  crypto,
  dgram,
  dns,
  'dns/promises': dns.promises,
  events,
  extension,
  fs,
  'fs/promises': fs.promises,
  http,
  gc,
  https,
  ipc,
  language,
  mime,
  net: {},
  os,
  path,
  // eslint-disable-next-line
  perf_hooks: {
    performance: globalThis.performance
  },
  process,
  querystring,
  stream,
  'stream/web': stream.web,
  // eslint-disable-next-line
  string_decoder,
  sys: util,
  test,
  timers,
  'timers/promises': timers.promises,
  tty: {
    isatty: () => false,
    WriteStream: util.IllegalConstructor,
    ReadStream: util.IllegalConstructor
  },
  util,
  url,
  vm,
  window,
  // eslint-disable-next-line
  worker_threads
}

const socketRuntimeModules = [
  'application',
  'extension',
  'gc',
  'ipc',
  'language',
  'mime',
  'window'
]

// alias
export const builtinModules = builtins

export function isBuiltin (name) {
  const originalName = name
  name = name.replace(/^(socket|node):/, '')

  if (
    socketRuntimeModules.includes(name) &&
    !originalName.startsWith('socket:')
  ) {
    return false
  }

  if (name in builtins) {
    return true
  }

  return false
}

/**
 * CommonJS module scope source wrapper.
 * @type {string}
 */
export const COMMONJS_WRAPPER = CommonJSModuleScope
  .toString()
  .split(/'module code'/)

/**
 * The main entry source origin.
 * @type {string}
 */
export const MAIN_SOURCE_ORIGIN = globalThis.location.href.startsWith('blob:')
  ? new URL(new URL(globalThis.location.href).pathname).href
  : globalThis.location.href

/**
 * Creates a `require` function from a source URL.
 * @param {URL|string} sourcePath
 * @return {function}
 */
export function createRequire (sourcePath) {
  return Module.createRequire(sourcePath)
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
   * A limited set of builtins exposed to CommonJS modules.
   * @type {object}
   */
  static builtins = builtins

  /**
   * Creates a `require` function from a source URL.
   * @param {URL|string} sourcePath
   * @return {function}
   */
  static createRequire (sourcePath) {
    if (!sourcePath) {
      return Module.main.createRequire()
    }

    return Module.from(sourcePath).createRequire()
  }

  /**
   * The main entry module, lazily created.
   * @type {Module}
   */
  static get main () {
    if (MAIN_SOURCE_ORIGIN in this.cache) {
      return this.cache[MAIN_SOURCE_ORIGIN]
    }

    const main = this.cache[MAIN_SOURCE_ORIGIN] = new Module(MAIN_SOURCE_ORIGIN)
    main.filename = main.id
    main.loaded = true
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
   * @param {string|URL|Module} [sourcePath]
   * @param {string|URL|Module} [parent]
   */
  static from (sourcePath, parent = null) {
    if (!sourcePath) {
      return Module.main
    } else if (sourcePath?.id) {
      return this.from(sourcePath.id, parent)
    } else if (sourcePath instanceof URL) {
      return this.from(String(sourcePath), parent)
    }

    if (!parent) {
      parent = Module.current
    }

    const id = String(parent
      ? new URL(sourcePath, parent.id)
      : new URL(sourcePath, MAIN_SOURCE_ORIGIN)
    )

    if (id in this.cache) {
      return this.cache[id]
    }

    const module = new Module(id, parent, sourcePath)
    this.cache[module.id] = module
    return module
  }

  /**
   * The module id, most likely a file name.
   * @type {string}
   */
  id = ''

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
  sourcePath = ''

  /**
   * `Module` class constructor.
   * @ignore
   */
  constructor (id, parent = null, sourcePath = null) {
    super()

    this.id = new URL(id || '', parent?.id || MAIN_SOURCE_ORIGIN).toString()
    this.parent = parent || null
    this.sourcePath = sourcePath || id

    if (!scope.previous && id !== MAIN_SOURCE_ORIGIN) {
      scope.previous = Module.main
    }

    if (!scope.current) {
      scope.current = this
    }

    this.addEventListener('error', (event) => {
      // @ts-ignore
      const { error } = event
      if (this.isMain) {
        // bubble error to globalThis, if possible
        if (typeof globalThis.dispatchEvent === 'function') {
          // @ts-ignore
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
    return this.id === MAIN_SOURCE_ORIGIN
  }

  /**
   * `true` if the module was loaded by name, not file path.
   * @type {boolean}
   */
  get isNamed () {
    return !this.isMain && !this.sourcePath?.startsWith('.') && !this.sourcePath?.startsWith('/')
  }

  /**
   * @type {URL}
   */
  get url () {
    return new URL(this.id)
  }

  /**
   * @type {string}
   */
  get pathname () {
    return new URL(this.id).pathname
  }

  /**
   * @type {string}
   */
  get path () {
    return path.dirname(this.pathname)
  }

  /**
   * Loads the module, synchronously returning `true` upon success,
   * otherwise `false`.
   * @return {boolean}
   */
  load () {
    const { isNamed, sourcePath } = this
    const prefixes = (process.env.SOCKET_MODULE_PATH_PREFIX || '').split(':')
    const urls = []

    if (this.loaded) {
      return true
    }

    Module.previous = Module.current
    Module.current = this

    // if module is named, and `prefixes` contains entries then
    // build possible search paths of `<prefix>/<name>` starting
    // from `.` up until `/` of the origin
    if (isNamed) {
      const name = sourcePath
      for (const prefix of prefixes) {
        let current = new URL('./', this.id).toString()

        do {
          const prefixURL = new URL(prefix, current)
          urls.push(new URL(name, prefixURL + '/').toString())
          current = new URL('..', current).toString()
        } while (new URL(current).pathname !== '/')

        const prefixURL = new URL(prefix, current)
        urls.push(new URL(name, prefixURL + '/').toString())
      }
    } else {
      urls.push(this.id)
    }

    for (const url of urls) {
      if (loadPackage(this, url)) {
        break
      }
    }

    if (this.loaded) {
      Module.previous = this
      Module.current = null
    }

    return this.loaded

    function loadPackage (module, url) {
      const hasTrailingSlash = url.endsWith('/')
      const urls = []
      const extname = path.extname(url)

      if (/\.(js|json|mjs|cjs)/.test(extname) && !hasTrailingSlash) {
        urls.push(url)
      } else {
        if (hasTrailingSlash) {
          urls.push(
            new URL('./index.js', url).toString(),
            new URL('./index.json', url).toString()
          )
        } else {
          urls.push(
            url + '.js',
            url + '.json',
            new URL('./index.js', url + '/').toString(),
            new URL('./index.json', url + '/').toString()
          )
        }
      }

      while (urls.length) {
        const filename = urls.shift()
        const response = request(filename)

        if (response.data !== null) {
          try {
            evaluate(module, filename, response.data)
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

      const response = request(
        url + (hasTrailingSlash ? 'package.json' : '/package.json')
      )

      if (response.data) {
        try {
          // @ts-ignore
          const packageJSON = JSON.parse(response.data)
          const filename = !packageJSON.exports
            ? path.resolve('/', url, packageJSON.main || packageJSON.browser)
            : path.resolve(url, (
              packageJSON.exports?.['.'] ||
              packageJSON.exports?.['./index.js'] ||
              packageJSON.exports?.['index.js']
            ))

          loadPackage(module, filename)
        } catch (error) {
          error.module = module
          module.dispatchEvent(new ErrorEvent('error', { error }))
        }
      }

      return module.loaded
    }

    function evaluate (module, filename, moduleSource) {
      const { protocol } = path.Path.from(filename)
      let dirname = path.dirname(filename)

      if (filename.startsWith(protocol) && !dirname.startsWith(protocol)) {
        dirname = protocol + '//' + dirname
      }

      if (path.extname(filename) === '.json') {
        module.id = new URL(filename, module.parent.id).toString()
        module.filename = filename

        try {
          module.exports = JSON.parse(moduleSource)
        } catch (error) {
          error.module = module
          module.dispatchEvent(new ErrorEvent('error', { error }))
          return false
        } finally {
          module.loaded = true
        }

        if (module.parent) {
          module.parent.children.push(module)
        }

        module.dispatchEvent(new Event('load'))
        return true
      }

      try {
        const source = Module.wrap(moduleSource)
        // eslint-disable-next-line no-new-func
        const define = new Function(`return ${source}`)()

        const oldId = module.id
        module.id = new URL(filename, module.parent.id).toString()
        module.filename = filename

        if (oldId !== module.id && Module.cache[module.id]) {
          module.exports = Module.cache[module.id].exports
          return true
        }

        Module.cache[module.id] = module

        // eslint-disable-next-line no-useless-call
        const promise = define.call(null,
          module.exports,
          module.createRequire(),
          module,
          filename,
          dirname,
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

    Object.assign(require, {
      cache: Module.cache,
      resolve (filename) {
        return resolve(filename, Array.from(Module.resolvers))
      }
    })

    return require

    function require (filename) {
      if (filename.startsWith('/') || filename.startsWith('\\')) {
        filename = filename.replace(process.cwd(), '')
      }

      const resolvers = Array.from(Module.resolvers)
      const result = resolve(filename, resolvers)

      if (typeof result === 'string' && result.length < 256 && !/^[\s|\n]+/.test(result)) {
        const name = result.replace(/^(socket|node):/, '')

        if (
          socketRuntimeModules.includes(name) &&
          !result.startsWith('socket:')
        ) {
          throw new ModuleNotFoundError(
            `Cannot require module ${filename} without 'socket:' prefix`,
            this.children
          )
        }

        if (isBuiltin(result)) {
          return builtins[name]
        }

        return module.require(name)
      }

      if (result !== undefined) {
        return result
      }

      throw new ModuleNotFoundError(
        `Cannot find module ${filename}`,
        module.children
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

export default Module

builtins.module = Module
