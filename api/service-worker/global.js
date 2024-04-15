import { ExtendableEvent, FetchEvent } from './events.js'
import { ServiceWorkerRegistration } from './registration.js'
import { createServiceWorker } from './instance.js'
import clients from './clients.js'
import state from './state.js'
import ipc from '../ipc.js'

// events
let onactivate = null
let onmessage = null
let oninstall = null
let onfetch = null

// this is set one time
let registration = null
let serviceWorker = null

export class ServiceWorkerGlobalScope {
  get isServiceWorkerScope () {
    return true
  }

  get ExtendableEvent () {
    return ExtendableEvent
  }

  get FetchEvent () {
    return FetchEvent
  }

  get serviceWorker () {
    return serviceWorker
  }

  get registration () {
    return registration
  }

  set registration (value) {
    if (!registration) {
      const info = { registration: value }

      serviceWorker = createServiceWorker(state.serviceWorker.state, info.registration)
      registration = new ServiceWorkerRegistration(info, serviceWorker)
    }
  }

  get clients () {
    return clients
  }

  get onactivate () {
    return onactivate
  }

  set onactivate (listener) {
    if (onactivate) {
      globalThis.removeEventListener('activate', onactivate)
    }

    onactivate = null

    if (typeof listener === 'function') {
      globalThis.addEventListener('activate', listener)
      onactivate = listener
    }
  }

  get onmessage () {
    return onmessage
  }

  set onmessage (listener) {
    if (onmessage) {
      globalThis.removeEventListener('message', onmessage)
    }

    onmessage = null

    if (typeof listener === 'function') {
      globalThis.addEventListener('message', listener)
      onmessage = listener
    }
  }

  get oninstall () {
    return oninstall
  }

  set oninstall (listener) {
    if (oninstall) {
      globalThis.removeEventListener('install', oninstall)
    }

    oninstall = null

    if (typeof listener === 'function') {
      globalThis.addEventListener('install', listener)
      oninstall = listener
    }
  }

  get onfetch () {
    return onfetch
  }

  set onfetch (listener) {
    if (fetch) {
      globalThis.removeEventListener('fetch', fetch)
    }

    onfetch = null

    if (typeof listener === 'function') {
      globalThis.addEventListener('fetch', listener)
      onfetch = listener
    }
  }

  async skipWaiting () {
    const result = await ipc.request('serviceWorker.skipWaiting', {
      id: state.id
    })

    if (result.err) {
      throw result.err
    }
  }
}

export default ServiceWorkerGlobalScope.prototype
