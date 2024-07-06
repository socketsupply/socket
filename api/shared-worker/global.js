// events
let onconnect = null

export class SharedWorkerGlobalScope {
  get isSharedWorkerScope () {
    return true
  }

  get onconnect () {
    return onconnect
  }

  set onconnect (listener) {
    if (onconnect) {
      globalThis.removeEventListener('connect', onconnect)
    }

    onconnect = null

    if (typeof listener === 'function') {
      globalThis.addEventListener('connect', listener)
      onconnect = listener
    }
  }
}

export default SharedWorkerGlobalScope.prototype
