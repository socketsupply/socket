import { showNotification, getNotifications } from './notification.js'
import ipc from '../ipc.js'

export class ServiceWorkerRegistration {
  #info = null
  #active = null
  #waiting = null
  #installing = null
  #onupdatefound = null

  constructor (info, serviceWorker) {
    this.#info = info

    // many states here just end up being the 'installing' state to the front end
    if (/installing|parsed|registered|registering/.test(serviceWorker.state)) {
      this.#installing = serviceWorker
    } else if (serviceWorker.state === 'installed') {
      this.#waiting = serviceWorker
    } else if (serviceWorker.state === 'activating' || serviceWorker.state === 'activated') {
      this.#active = serviceWorker
    }

    serviceWorker.addEventListener('statechange', (event) => {
      const { state } = event.target

      if (state === 'installing') {
        this.#active = null
        this.#waiting = null
        this.#installing = serviceWorker
      }

      if (state === 'installed') {
        this.#active = null
        this.#waiting = serviceWorker
        this.#installing = null
      }

      if (state === 'activating' || state === 'activated') {
        this.#active = serviceWorker
        this.#waiting = null
        this.#installing = null
      }
    })
  }

  get [Symbol.for('socket.runtime.ServiceWorkerRegistration.info')] () {
    return this.#info?.registration ?? null
  }

  get scope () {
    return this.#info.registration.scope
  }

  get updateViaCache () {
    return 'none'
  }

  get installing () {
    return this.#installing
  }

  get waiting () {
    return this.#waiting
  }

  get active () {
    return this.#active
  }

  get onupdatefound () {
    return this.#onupdatefound
  }

  set onupdatefound (onupdatefound) {
    if (this.#onupdatefound) {
      this.removeEventListener('updatefound', this.#onupdatefound)
    }

    this.#onupdatefound = null

    if (typeof onupdatefound === 'function') {
      this.addEventListener('updatefound', this.#onupdatefound)
      this.#onupdatefound = onupdatefound
    }
  }

  get navigationPreload () {
    return null
  }

  async getNotifications () {
    return await getNotifications(this)
  }

  async showNotification (title, options) {
    return await showNotification(this, title, options)
  }

  async unregister () {
    const result = await ipc.request('serviceWorker.unregister', {
      scope: this.scope
    })

    if (result.err) {
      throw result.err
    }
  }

  async update () {
  }
}

if (typeof globalThis.ServiceWorkerRegistration === 'function') {
  Object.setPrototypeOf(
    ServiceWorkerRegistration.prototype,
    globalThis.ServiceWorkerRegistration.prototype
  )
}

export default ServiceWorkerRegistration
