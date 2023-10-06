/* global Event */
/**
 * @module extension
 *
 * @example
 * import extension from 'socket:extension'
 * const ex = await extension.load('native_extension')
 * const result = ex.binding.method('argument')
 */
import ipc from './ipc.js'

const $loaded = Symbol('loaded')

/**
 * @typedef {{ abi: number, version: string, description: string }} ExtensionInfo
 */

/**
 * @typedef {{ abi: number, loaded: number }} ExtensionStats
 */

/**
 * A interface for a native extension.
 * @template {Record<string, any> T}
 */
export class Extension extends EventTarget {
  /**
   * Load an extension by name.
   * @template {Record<string, any> T}
   * @param {string} name
   * @param {ExtensionLoadOptions} [options]
   * @return {Promise<Extension<T>>}
   */
  static async load (name, options) {
    options = { name, ...options }

    if (Array.isArray(options.allow)) {
      options.allow = options.allow.join(',')
    }

    const result = await ipc.request('extension.load', options)

    if (result.err) {
      throw new Error('Failed to load extensions', { cause: result.err })
    }

    const extension = new this(name, result.data, options)
    extension[$loaded] = true
    return extension
  }

  /**
   * Provides current stats about the loaded extensions.
   * @return {Promise<ExtensionStats>}
   */
  static async stats () {
    const result = await ipc.request('extension.stats')
    return result.data ?? null
  }

  /**
   * The name of the extension
   * @type {string?}
   */
  name = null

  /**
   * The version of the extension
   * @type {string?}
   */
  version = null

  /**
   * The description of the extension
   * @type {string?}
   */
  description = null

  /**
   * The abi of the extension
   * @type {number}
   */
  abi = 0

  /**
   * @type {object}
   */
  options = {}

  /**
   * @type {T}
   */
  binding = null

  /**
   * `Extension` class constructor.
   * @param {string} name
   * @param {ExtensionInfo} info
   * @param {ExtensionLoadOptions} [options]
   */
  constructor (name, info, options = null) {
    super()

    this.abi = info?.abi || 0
    this.name = name
    this.version = info?.version || null
    this.description = info?.description || null
    this.options = options
    this.binding = ipc.createBinding(options?.binding?.name || this.name, {
      ...options?.binding,
      extension: this,
      default: 'request'
    })

    this[$loaded] = false
  }

  /**
   * `true` if the extension was loaded, otherwise `false`
   * @type {boolean}
   */
  get loaded () {
    return this[$loaded]
  }

  /**
   * Unloads the loaded extension.
   * @throws Error
   */
  async unload () {
    if (!this.loaded) {
      throw new Error('Extension is not loaded')
    }

    const { name } = this
    const result = await ipc.request('extension.unload', { name })

    if (result.err) {
      throw new Error('Failed to unload extension', { cause: result.err })
    }

    this[$loaded] = false
    this.dispatchEvent(new Event('unload'))

    return result.data
  }
}

/**
 * @typedef {{ allow: string[] | string }} ExtensionLoadOptions
 */

/**
 * Load an extension by name.
 * @template {Record<string, any> T}
 * @param {string} name
 * @param {ExtensionLoadOptions} [options]
 * @return {Promise<Extension<T>>}
 */
export async function load (name, options = {}) {
  return await Extension.load(name, options)
}

/**
 * Provides current stats about the loaded extensions.
 * @return {Promise<ExtensionStats>}
 */
export async function stats () {
  return await Extension.stats()
}

export default {
  load,
  stats
}
