import { ModuleNotFoundError } from '../errors.js'

// eslint-disable-next-line
import _async, {
  AsyncLocalStorage,
  AsyncResource,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook,
  AsyncHook
} from '../async.js'

// eslint-disable-next-line
import * as ai from '../ai.js'
import * as application from '../application.js'
import assert from '../assert.js'
import * as buffer from '../buffer.js'
// eslint-disable-next-line
import * as child_process from '../child_process.js'
import console from '../console.js'
import * as constants from '../constants.js'
import * as crypto from '../crypto.js'
import * as dgram from '../dgram.js'
import * as diagnostics from '../diagnostics.js'
import * as dns from '../dns.js'
import events from '../events.js'
import errno from '../errno.js'
import * as extension from '../extension.js'
import * as fs from '../fs.js'
import * as gc from '../gc.js'
import * as http from '../http.js'
import * as https from '../https.js'
import * as ipc from '../ipc.js'
import * as language from '../language.js'
import * as location from '../location.js'
import * as mime from '../mime.js'
import * as network from '../network.js'
import * as os from '../os.js'
import { posix as path } from '../path.js'
import process from '../process.js'
import * as querystring from '../querystring.js'
import * as serviceWorker from '../service-worker.js'
import stream from '../stream.js'
// eslint-disable-next-line
import * as string_decoder from '../string_decoder.js'
import test from '../test.js'
import * as timers from '../timers.js'
import * as tty from '../tty.js'
import * as url from '../url.js'
import util from '../util.js'
import vm from '../vm.js'
import * as window from '../window.js'
// eslint-disable-next-line
import * as worker_threads from '../worker_threads.js'

/**
 * A mapping of builtin modules
 * @type {object}
 */
export const builtins = {}

/**
 * Defines a builtin module by name making a shallow copy of the
 * module exports.
 * @param {string}
 * @param {object} exports
 */
export function defineBuiltin (name, exports, copy = true) {
  if (exports && typeof exports === 'object') {
    if (copy) {
      builtins[name] = { ...exports }
      delete builtins[name].default
    } else {
      builtins[name] = exports
    }
  } else if (typeof exports === 'string' && exports in builtins) {
    // alias
    Object.defineProperty(builtins, name, {
      configurable: true,
      enumerable: true,
      get: () => builtins[exports]
    })
  } else {
    builtins[name] = exports
  }
}

// node.js compat modules
defineBuiltin('async_context', {
  AsyncLocalStorage,
  AsyncResource
})
defineBuiltin('async_hooks', {
  AsyncLocalStorage,
  AsyncResource,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook,
  AsyncHook
})
defineBuiltin('ai', ai)
defineBuiltin('assert', assert, false)
defineBuiltin('buffer', buffer, false)
defineBuiltin('console', console, false)
defineBuiltin('constants', constants)
// eslint-disable-next-line
defineBuiltin('child_process', child_process)
defineBuiltin('crypto', crypto)
defineBuiltin('dgram', dgram)
defineBuiltin('diagnostics_channel', diagnostics)
defineBuiltin('dns', dns)
defineBuiltin('dns/promises', dns.promises)
defineBuiltin('events', events, false)
defineBuiltin('fs', fs)
defineBuiltin('fs/promises', fs.promises)
defineBuiltin('http', http)
defineBuiltin('https', https)
defineBuiltin('net', {})
defineBuiltin('os', os)
defineBuiltin('path', path)
defineBuiltin('perf_hooks', { performance: globalThis.performance })
defineBuiltin('process', process, false)
defineBuiltin('querystring', querystring)
defineBuiltin('stream', stream, false)
defineBuiltin('stream/web', stream.web)
// eslint-disable-next-line
defineBuiltin('string_decoder', string_decoder)
defineBuiltin('sys', util)
defineBuiltin('test', test, false)
defineBuiltin('timers', timers)
defineBuiltin('timers/promises', timers.promises)
defineBuiltin('tty', tty)
defineBuiltin('util', util)
defineBuiltin('util/types', util.types)
defineBuiltin('url', url)
defineBuiltin('vm', vm)
// eslint-disable-next-line
defineBuiltin('worker_threads', worker_threads)
// unsupported, but stubbed as entries
defineBuiltin('v8', {})
defineBuiltin('zlib', {})

// runtime modules
// eslint-disable-next-line
defineBuiltin('async', _async)
defineBuiltin('application', application)
defineBuiltin('commonjs/builtins', builtins, false)
defineBuiltin('errno', errno)
defineBuiltin('extension', extension)
defineBuiltin('gc', gc)
defineBuiltin('ipc', ipc)
defineBuiltin('language', language)
defineBuiltin('location', location)
defineBuiltin('mime', mime)
defineBuiltin('network', network)
defineBuiltin('service-worker', serviceWorker)
defineBuiltin('window', window)

/**
 * Known runtime specific builtin modules.
 * @type {Set<string>}
 */
export const runtimeModules = new Set([
  'async',
  'application',
  'commonjs',
  'commonjs/builtins',
  'commonjs/cache',
  'commonjs/loader',
  'commonjs/module',
  'commonjs/package',
  'commonjs/require',
  'extension',
  'errno',
  'gc',
  'ipc',
  'language',
  'location',
  'mime',
  'network',
  'service-worker',
  'window'
])

/**
 * Predicate to determine if a given module name is a builtin module.
 * @param {string} name
 * @param {{ builtins?: object }}
 * @return {boolean}
 */
export function isBuiltin (name, options = null) {
  const originalName = name
  name = name.replace(/^(socket|node):/, '')

  if (
    runtimeModules.has(name) &&
    !originalName.startsWith('socket:')
  ) {
    return false
  }

  if (options?.builtins && typeof options.builtins === 'object') {
    return name in builtins
  } else if (name in builtins) {
    return true
  }

  return false
}

/**
 * Gets a builtin module by name.
 * @param {string} name
 * @param {{ builtins?: object }} [options]
 * @return {any}
 */
export function getBuiltin (name, options = null) {
  const originalName = name
  name = name.replace(/^(socket|node):/, '')

  if (
    runtimeModules.has(name) &&
    !originalName.startsWith('socket:')
  ) {
    throw new ModuleNotFoundError(
      `Cannot find builtin module '${originalName}`
    )
  }

  if (options?.builtins && typeof options.builtins === 'object') {
    if (name in options.builtins) {
      return options.builtins[name]
    }
  }

  if (name in builtins) {
    return builtins[name]
  }

  throw new ModuleNotFoundError(
    `Cannot find builtin module '${originalName}`
  )
}

export default builtins
