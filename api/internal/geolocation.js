/* global EventTarget, CustomEvent, GeolocationCoordinates, GeolocationPosition, GeolocationPositionError */
import hooks from 'socket:hooks'
import ipc from '../ipc.js'
import os from '../os.js'

const isAndroid = os.platform() === 'android'
const isApple = os.platform() === 'darwin'

const watchers = {}

let currentWatchIdenfifier = 0

/**
 * @ignore
 */
class Watcher extends EventTarget {
  /**
   * @type {number}
   */
  identifier = 0

  /**
   * @type {function(CustomEvent): undefined}
   */
  // eslint-disable-next-line
  listener = (event) => void event

  constructor (identifier) {
    super()

    this.identifier = identifier
    this.listener = (event) => {
      if (event.detail?.params?.watch?.identifier === identifier) {
        const position = createGeolocationPosition(event.detail.params)
        this.dispatchEvent(new CustomEvent('position', { detail: position }))
      }
    }

    globalThis.addEventListener('data', this.listener)
  }

  stop () {
    globalThis.removeEventListener('data', this.listener)
  }
}

/**
 * @ignore
 */
function createGeolocationPosition (data) {
  const coords = Object.create(GeolocationCoordinates.prototype, {
    latitude: {
      configurable: false,
      writable: false,
      value: data.coords.latitude
    },

    longitude: {
      configurable: false,
      writable: false,
      value: data.coords.longitude
    },

    altitude: {
      configurable: false,
      writable: false,
      value: data.coords.altitude
    },

    accuracy: {
      configurable: false,
      writable: false,
      value: data.coords.accuracy
    },

    altitudeAccuracy: {
      configurable: false,
      writable: false,
      value: data.coords.altitudeAccuracy ?? null
    },

    floorLevel: {
      configurable: false,
      writable: false,
      value: data.coords.floorLevel ?? null
    },

    heading: {
      configurable: false,
      writable: false,
      value: data.coords.heading ?? null
    },

    speed: {
      configurable: false,
      writable: false,
      value: data.coords.speed <= 0 ? null : data.coords.speed
    }
  })

  return Object.create(GeolocationPosition.prototype, {
    coords: { configurable: false, writable: false, value: coords },
    timestamp: { configurable: false, writable: false, value: Date.now() }
  })
}

/**
 * @ignore
 * @param {string} name}
 * @return {function}
 */
function getPlatformFunction (name) {
  if (!globalThis.window?.navigator?.geolocation?.[name]) return null
  const value = globalThis.window.navigator.geolocation[name]
  return value.bind(globalThis.navigator.geolocation)
}

/**
 * @ignore
 */
export const platform = {
  getCurrentPosition: getPlatformFunction('getCurrentPosition'),
  watchPosition: getPlatformFunction('watchPosition'),
  clearWatch: getPlatformFunction('clearWatch')
}

/**
 * Get the current position of the device.
 * @param {function(GeolocationPosition)} onSuccess
 * @param {onError(Error)} onError
 * @param {object=} options
 * @param {number=} options.timeout
 * @return {Promise}
 */
export async function getCurrentPosition (
  onSuccess,
  onError,
  options = { timeout: null }
) {
  if (isAndroid) {
    await new Promise((resolve) => hooks.onReady(resolve))

    const result = await ipc.send('permissions.request', { name: 'geolocation' })

    if (result.err) {
      if (typeof onError === 'function') {
        onError(result.err)
        return
      } else {
        throw result.err
      }
    }
  }

  if (!isApple) {
    return platform.getCurrentPosition(...arguments)
  }

  if (arguments.length === 0) {
    throw new TypeError(
      'Failed to execute \'getCurrentPosition\' on \'Geolocation\': ' +
      '1 argument required, but only 0 present.'
    )
  }

  if (typeof onSuccess !== 'function') {
    throw new TypeError(
      'Failed to execute \'getCurrentPosition\' on \'Geolocation\': ' +
      'parameter 1 is not of type \'function\'.'
    )
  }

  let timer = null
  let didTimeout = false
  if (Number.isFinite(options?.timeout)) {
    timer = setTimeout(() => {
      didTimeout = true
      const error = Object.create(GeolocationPositionError.prototype, {
        code: { value: GeolocationPositionError.TIMEOUT }
      })

      if (typeof onError === 'function') {
        onError(error)
      } else {
        throw error
      }
    }, options.timeout)
  }

  const result = await ipc.send('geolocation.getCurrentPosition')

  if (didTimeout) {
    return
  }

  clearTimeout(timer)

  if (result.err) {
    if (typeof onError === 'function') {
      onError(result.err)
      return
    } else {
      throw result.err
    }
  }

  const position = createGeolocationPosition(result.data)

  if (typeof onSuccess === 'function') {
    onSuccess(position)
  }
}

/**
 * Register a handler function that will be called automatically each time the
 * position of the device changes. You can also, optionally, specify an error
 * handling callback function.
 * @param {function(GeolocationPosition)} onSuccess
 * @param {function(Error)} onError
 * @param {object=} [options]
 * @param {number=} [options.timeout = null]
 * @return {number}
 */
export function watchPosition (
  onSuccess,
  onError,
  options = { timeout: null }
) {
  if (isAndroid) {
    const result = ipc.sendSync('permissions.request', { name: 'geolocation' })

    if (result.err) {
      if (typeof onError === 'function') {
        onError(result.err)
        return
      } else {
        throw result.err
      }
    }
  }

  if (!isApple) {
    return platform.watchPosition(...arguments)
  }

  if (arguments.length === 0) {
    throw new TypeError(
      'Failed to execute \'getCurrentPosition\' on \'Geolocation\': ' +
      '1 argument required, but only 0 present.'
    )
  }

  if (typeof onSuccess !== 'function') {
    throw new TypeError(
      'Failed to execute \'getCurrentPosition\' on \'Geolocation\': ' +
      'parameter 1 is not of type \'function\'.'
    )
  }

  const identifier = currentWatchIdenfifier + 1

  let timer = null
  let didTimeout = false
  if (Number.isFinite(options?.timeout)) {
    timer = setTimeout(() => {
      didTimeout = true
      const error = Object.create(GeolocationPositionError.prototype, {
        code: { value: GeolocationPositionError.TIMEOUT }
      })

      if (typeof onError === 'function') {
        onError(error)
      } else {
        throw error
      }
    }, options.timeout)
  }

  ipc.send('geolocation.watchPosition', { id: identifier }).then((result) => {
    if (result.err) {
      if (typeof onError === 'function') {
        onError(result.err)
        return
      } else {
        throw result.err
      }
    }

    if (didTimeout) {
      clearWatch(identifier)
      return
    }

    const watcher = new Watcher(identifier)
    watchers[identifier] = watcher
    watcher.addEventListener('position', (event) => {
      const detail = /** @type {CustomEvent} */ (event)
      clearTimeout(timer)
      onSuccess(/** @type {GeolocationPosition} */ (detail))
    })
  })

  currentWatchIdenfifier = identifier
  return identifier
}

/**
 * Unregister location and error monitoring handlers previously installed
 * using `watchPosition`.
 * @param {number} id
 */
export function clearWatch (id) {
  if (!isApple) {
    return platform.clearWatch(...arguments)
  }

  watchers[id]?.stop()
  delete watchers[id]

  ipc.send('geolocation.clearWatch', { id })
}

export default {
  getCurrentPosition,
  watchPosition,
  clearWatch
}
