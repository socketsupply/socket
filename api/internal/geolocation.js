/* global EventTarget, GeolocationCoordinates, GeolocationPosition, GeolocationPositionError */
import ipc from '../ipc.js'
import os from '../os.js'

const isApple = os.platform() === 'darwin'
const watchers = {}

let currentWatchIdenfifier = 0

class Watcher extends EventTarget {
  constructor (identifier) {
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

export const platform = {
  getCurrentPosition: globalThis.navigator?.getCurrentPosition?.bind(globalThis.navigator),
  watchPosition: globalThis.navigator?.watchPosition?.bind(globalThis.navigator),
  clearWatch: globalThis.navigator?.clearWatch?.bind(globalThis.navigator)
}

export async function getCurrentPosition (onSuccess, onError, options = {}) {
  if (!isApple) {
    return platform.getCurrentPosition(...arguments)
  }

  if (typeof onSuccess !== 'function') {
    throw new TypeError(
      'Failed to execute \'getCurrentPosition\' on \'Geolocation\': ' +
      '1 argument required, but only 0 present.'
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

export function watchPosition (onSuccess, onError, options = {}) {
  if (!isApple) {
    return platform.watchPosition(...arguments)
  }

  if (typeof onSuccess !== 'function') {
    throw new TypeError(
      'Failed to execute \'getCurrentPosition\' on \'Geolocation\': ' +
      '1 argument required, but only 0 present.'
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
      clearTimeout(timer)
      onSuccess(event.detail)
    })
  })

  currentWatchIdenfifier = identifier
  return identifier
}

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
