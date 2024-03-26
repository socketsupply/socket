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
import application from '../application.js'
import assert from '../assert.js'
import * as buffer from '../buffer.js'
// eslint-disable-next-line
import child_process from '../child_process.js'
import console from '../console.js'
import constants from '../constants.js'
import crypto from '../crypto.js'
import dgram from '../dgram.js'
import dns from '../dns.js'
import events from '../events.js'
import extension from '../extension.js'
import fs from '../fs.js'
import gc from '../gc.js'
import http from '../http.js'
import https from '../https.js'
import ipc from '../ipc.js'
import language from '../language.js'
import location from '../location.js'
import mime from '../mime.js'
import network from '../network.js'
import os from '../os.js'
import { posix as path } from '../path.js'
import process from '../process.js'
import querystring from '../querystring.js'
import stream from '../stream.js'
// eslint-disable-next-line
import string_decoder from '../string_decoder.js'
import test from '../test.js'
import timers from '../timers.js'
import url from '../url.js'
import util from '../util.js'
import vm from '../vm.js'
import window from '../window.js'
// eslint-disable-next-line
import worker_threads from '../worker_threads.js'

/**
 * A mapping of builtin modules
 * @type {object}
 */
export const builtins = {
  // eslint-disable-next-line
  'async': _async,
  // eslint-disable-next-line
  async_context: {
    AsyncLocalStorage,
    AsyncResource
  },
  // eslint-disable-next-line
  async_hooks: {
    AsyncLocalStorage,
    AsyncResource,
    executionAsyncResource,
    executionAsyncId,
    triggerAsyncId,
    createHook,
    AsyncHook
  },
  application,
  assert,
  buffer,
  console,
  constants,
  // eslint-disable-next-line
  child_process,
  crypto,
  dgram,
  dns,
  'dns/promises': dns.promises,
  events,
  extension,
  fs,
  'fs/promises': fs.promises,
  http,
  https,
  gc,
  ipc,
  language,
  location,
  mime,
  net: {},
  network,
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
      `Cannnot find builtin module '${originalName}`
    )
  }

  if (name in builtins) {
    return builtins[name]
  }

  throw new ModuleNotFoundError(
    `Cannnot find builtin module '${originalName}`
  )
}

export default builtins
