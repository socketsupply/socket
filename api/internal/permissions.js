/* global EventTarget, CustomEvent, Event */
/**
 * @module Permissions
 * This module provides an API for querying and requesting permissions.
 */
import { IllegalConstructorError } from '../errors.js'
import Notification from '../notification.js'
import Enumeration from '../enumeration.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'
import gc from '../gc.js'
import os from '../os.js'

/**
 * The 'permissionchange' event name.
 * @ignore
 * @type {string}
 */
const PERMISSION_CHANGE_EVENT = 'permissionchange'

/**
 * @typedef {{ name: string }} PermissionDescriptor
 */

const isAndroid = os.platform() === 'android'
const isApple = os.platform() === 'darwin' || os.platform() === 'ios'
const isLinux = os.platform() === 'linux'

/**
 * Get a bound platform `navigator.permissions` function.
 * @ignore
 * @param {string} name}
 * @return {function}
 */
function getPlatformFunction (name) {
  if (!globalThis.window?.navigator?.permissions?.[name]) return null
  const value = globalThis.window.navigator.permissions[name]
  return value.bind(globalThis.navigator.permissions)
}

/**
 * Native platform functions
 * @ignore
 */
const platform = {
  query: getPlatformFunction('query')
}

/**
 * An enumeration of the permission types.
 * - 'geolocation'
 * - 'notifications'
 * - 'push'
 * - 'persistent-storage'
 * - 'midi'
 * - 'storage-access'
 * @type {Enumeration}
 * @ignore
 */
export const types = Enumeration.from([
  'geolocation',
  'notifications',
  'push',
  'persistent-storage',
  'midi',
  'storage-access'
])

/**
 * A container that provides the state of an object and an event handler
 * for monitoring changes permission changes.
 * @ignore
 */
class PermissionStatus extends EventTarget {
  #removePermissionChangeListener = null
  #subscribed = true
  #onchange = null
  #signal = null
  #state = null
  #name = null

  /**
   * `PermissionStatus` class constructor.
   * @param {string} name
   * @param {string} initialState
   * @param {object=} [options]
   * @param {?AbortSignal} [options.signal = null]
   */
  constructor (name, initialState, options = { signal: null }) {
    super()
    this.#name = name
    this.#state = initialState
    this.#signal = options?.signal ?? null

    this.#removePermissionChangeListener = hooks.onPermissionChange((event) => {
      const { detail } = event
      if (this.#subscribed === false) {
        this.#removePermissionChangeListener()
        this.#removePermissionChangeListener = null
        return
      }

      if (detail.name === name && detail.state !== this.#state) {
        this.#state = detail.state
        this.dispatchEvent(new Event('change'))
      }
    })

    if (this.#signal?.aborted === true) {
      this.removePermissionChangeListener()
    }

    if (typeof this.#signal?.addEventListener === 'function') {
      this.#signal.addEventListener('abort', () => {
        this.#removePermissionChangeListener()
        this.#removePermissionChangeListener = null
        this.unsubscribe()
      }, { once: true })
    }

    gc.ref(this)
  }

  /**
   * String tag for `PermissionStatus`.
   * @ignore
   */
  get [Symbol.toStringTag] () {
    return 'PermissionStatus'
  }

  /**
   * The name of this permission this status is for.
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * The current state of the permission status.
   * @type {string}
   */
  get state () {
    return this.#state
  }

  /**
   * Level 0 event target 'change' event listener accessor
   * @type {function(Event)}
   */
  get onchange () { return this.#onchange }
  set onchange (onchange) {
    if (typeof this.#onchange === 'function') {
      this.removeEventListener('change', this.#onchange)
    }

    if (typeof onchange === 'function') {
      this.#onchange = onchange
      this.addEventListener('change', onchange)
    }
  }

  /**
   * Non-standard method for unsubscribing to status state updates.
   * @ignore
   */
  unsubscribe () {
    this.#subscribed = false
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] () {
    return {
      args: [this.removePermissionChangeListener],
      handle (removePermissionChangeListener) {
        removePermissionChangeListener()
      }
    }
  }
}

/**
 * Query for a permission status.
 * @param {PermissionDescriptor} descriptor
 * @param {object=} [options]
 * @param {?AbortSignal} [options.signal = null]
 * @return {Promise<PermissionStatus>}
 */
export async function query (descriptor, options) {
  if (arguments.length === 0) {
    throw new TypeError(
      'Failed to execute \'query\' on \'Permissions\': ' +
      '1 argument required, but only 0 present.'
    )
  }

  if (!descriptor || typeof descriptor !== 'object') {
    throw new TypeError(
      'Failed to execute \'query\' on \'Permissions\': ' +
      'parameter 1 is not of type \'object\'.'
    )
  }

  const { name } = descriptor

  if (name === undefined) {
    throw new TypeError(
      'Failed to execute \'query\' on \'Permissions\': ' +
      'Failed to read the \'name\' property from \'PermissionDescriptor\': ' +
      'Required member is undefined.'
    )
  }

  if (typeof name !== 'string' || name.length === 0 || !types.contains(name)) {
    throw new TypeError(
      'Failed to execute \'query\' on \'Permissions\': ' +
      'Failed to read the \'name\' property from \'PermissionDescriptor\': ' +
      `The provided value '${name}' is not a valid enum value of type PermissionName.`
    )
  }

  if (!isAndroid && !isApple) {
    if (isLinux) {
      if (name === 'notifications' || name === 'push') {
        return new PermissionStatus(name, Notification.permission)
      }
    }

    return platform.query(descriptor)
  }

  const result = await ipc.request('permissions.query', { name, signal: options?.signal })

  if (result.err) {
    throw result.err
  }

  return new PermissionStatus(name, result.data.state, options)
}

/**
 * Request a permission to be granted.
 * @param {PermissionDescriptor} descriptor
 * @param {object=} [options]
 * @param {?AbortSignal} [options.signal = null]
 * @return {Promise<PermissionStatus>}
 */
export async function request (descriptor, options) {
  if (arguments.length === 0) {
    throw new TypeError(
      'Failed to execute \'request\' on \'Permissions\': ' +
      '1 argument required, but only 0 present.'
    )
  }

  if (!descriptor || typeof descriptor !== 'object') {
    throw new TypeError(
      'Failed to execute \'request\' on \'Permissions\': ' +
      'parameter 1 is not of type \'object\'.'
    )
  }

  const { name } = descriptor

  if (name === undefined) {
    throw new TypeError(
      'Failed to execute \'request\' on \'Permissions\': ' +
      'Failed to read the \'name\' property from \'PermissionDescriptor\': ' +
      'Required member is undefined.'
    )
  }

  if (typeof name !== 'string' || name.length === 0 || !types.contains(name)) {
    throw new TypeError(
      'Failed to execute \'request\' on \'Permissions\': ' +
      'Failed to read the \'name\' property from \'PermissionDescriptor\': ' +
      `The provided value '${name}' is not a valid enum value of type PermissionName.`
    )
  }

  if (isLinux) {
    if (name === 'notifications' || name === 'push') {
      const currentState = Notification.permission
      // `Notification.requestPermission` will use the native
      // `requestPermission` API internally, so this won't be a cycle
      const state = await Notification.requestPermission()

      if (currentState !== state) {
        const globalEvent = new CustomEvent(PERMISSION_CHANGE_EVENT, {
          detail: { name, state }
        })

        queueMicrotask(() => {
          globalThis.dispatchEvent(globalEvent)
        })
      }

      return new PermissionStatus(name, state, options)
    }

    return new PermissionStatus(name, 'prompt', options)
  }

  const { signal } = options || {}

  if (options?.signal) {
    delete options.signal
  }

  const result = await ipc.request('permissions.request', { ...options, name, signal })

  if (result.err) {
    throw result.err
  }

  const globalEvent = new CustomEvent(PERMISSION_CHANGE_EVENT, {
    detail: {
      name,
      state: result.data.state
    }
  })

  queueMicrotask(() => {
    globalThis.dispatchEvent(globalEvent)
  })

  return new PermissionStatus(name, result.data.state, options)
}

/**
 * An interface for querying and revoking permissions.
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Permissions}
 */
class Permissions {
  /**
   * Returns the state of a user permission on the global scope.
   * @type {function(PermissionDescriptor): Promise<PermissionStatus>}
   */
  query = query

  /**
   * Request a permission to be granted.
   * @type {function(PermissionDescriptor): Promise<PermissionStatus>}
   */
  request = request

  /**
   * `Permissions` class constructor. This interface is
   * not constructable.
   * @ignore
   */
  constructor () {
    throw new IllegalConstructorError()
  }
}

export default Object.assign(Object.create(Permissions.prototype), {
  query,
  request
})
