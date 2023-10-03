/* global EventTarget, Event */
import { IllegalConstructorError } from '../errors.js'
import ipc from '../ipc.js'
import os from '../os.js'

const isAndroid = os.platform() === 'android'

class PermissionStatus extends EventTarget {
  #onchange = null
  #state = null
  #name = null

  // eslint-disable-next-line
  [Symbol.toStringTag] = 'PermissionStatus'

  constructor (name, subscribe) {
    super()
    this.#name = name
    subscribe((state) => {
      if (this.#state !== state) {
        this.#state = state
        this.dispatchEvent(new Event('change'))
      }
    })
  }

  get name () {
    return this.#name
  }

  get state () {
    return this.#state
  }

  set onchange (onchange) {
    if (typeof this.#onchange === 'function') {
      this.removeEventListener('change', this.#onchange)
    }

    if (typeof onchange === 'function') {
      this.#onchange = onchange
      this.addEventListener('change', onchange)
    }
  }

  get onchange () {
    return this.#onchange
  }
}

/**
 * @ignore
 * @param {string} name}
 * @return {function}
 */
function getPlatformFunction (name) {
  if (!globalThis.window?.navigator?.permissions?.[name]) return null
  const value = globalThis.window.navigator.permissions[name]
  return value.bind(globalThis.navigator.permissions)
}

const platform = {
  query: getPlatformFunction('query')
}

/**
 * @param {{ name: string }} descriptor
 * @return {Promise<PermissionStatus>}
 */
export async function query (descriptor) {
  if (!isAndroid) {
    return platform.query(descriptor)
  }

  const types = [
    'geolocation',
    'notifications',
    'push',
    'persistent-storage',
    'midi',
    'storage-access'
  ]

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

  if (typeof name !== 'string' || name.length === 0 || !types.includes(name)) {
    throw new TypeError(
      'Failed to execute \'query\' on \'Permissions\': ' +
      'Failed to read the \'name\' property from \'PermissionDescriptor\': ' +
      `The provided value '${name}' is not a valid enum value of type PermissionName.`
    )
  }

  const result = await ipc.send('permissions.query', { name })
  const status = new PermissionStatus(name, async (update) => {
    queueMicrotask(() => update(result.data.state))
  })

  if (result.err) {
    throw result.err
  }

  return status
}

class Permissions {
  constructor () {
    throw new IllegalConstructorError()
  }
}

export default Object.assign(Object.create(Permissions.prototype), { query })
