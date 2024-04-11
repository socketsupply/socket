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
import stream from '../stream.js'
// eslint-disable-next-line
import * as string_decoder from '../string_decoder.js'
import * as test from '../test.js'
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
export function define (name, exports, copy = true) {
  if (exports && typeof exports === 'object') {
    if (copy) {
      builtins[name] = { ...exports }
      delete builtins[name].default
    } else {
      builtins[name] = exports
    }
  } else {
    builtins[name] = exports
  }
}

// eslint-disable-next-line
define('async', _async)
define('async_context', {
  AsyncLocalStorage,
  AsyncResource
})
define('async_hooks', {
  AsyncLocalStorage,
  AsyncResource,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook,
  AsyncHook
})

define('application', application)
define('assert', assert, false)
define('buffer', buffer, false)
define('console', console, false)
define('constants', constants)
// eslint-disable-next-line
define('child_process', child_process)
define('crypto', crypto)
define('dgram', dgram)
define('diagnostics_channel', diagnostics)
define('dns', dns)
define('dns/promises', dns.promises)
define('events', events, false)
define('extension', extension)
define('fs', fs)
define('fs/promises', fs.promises)
define('http', http)
define('https', https)
define('gc', gc)
define('ipc', ipc)
define('language', language)
define('location', location)
define('mime', mime)
define('net', {})
define('network', network)
define('os', os)
define('path', path)
define('perf_hooks', { performance: globalThis.performance })
define('process', process, false)
define('querystring', querystring)
define('stream', stream, false)
define('stream/web', stream.web)
// eslint-disable-next-line
define('string_decoder', string_decoder)
define('sys', util)
define('test', test)
define('timers', timers)
define('timers/promises', timers.promises)
define('tty', tty)
define('util', util)
define('util/types', util.types)
define('url', url)
define('vm', vm)
define('window', window)
// eslint-disable-next-line
define('worker_threads', worker_threads)

// unsupported, but stubbed as entries
define('v8', {})
define('zlib', {})

/**
 * Known runtime specific builtin modules.
 * @type {string[]}
 */
export const runtimeModules = [
  'async',
  'application',
  'extension',
  'gc',
  'ipc',
  'language',
  'location',
  'mime',
  'network',
  'window'
]

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
    runtimeModules.includes(name) &&
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
 * @param {{ builtins?: object }}
 * @return {any}
 */
export function getBuiltin (name, options = null) {
  const originalName = name
  name = name.replace(/^(socket|node):/, '')

  if (
    runtimeModules.includes(name) &&
    !originalName.startsWith('socket:')
  ) {
    throw new ModuleNotFoundError(
      `Cannot find builtin module '${originalName}`
    )
  }

  if (name in builtins) {
    return builtins[name]
  }

  throw new ModuleNotFoundError(
    `Cannot find builtin module '${originalName}`
  )
}

export default builtins
