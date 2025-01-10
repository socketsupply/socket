import { Notification, NotificationOptions } from '../notification.js'
import { NotAllowedError } from '../errors.js'
import permissions from '../internal/permissions.js'

const observedNotifications = new Set()
const channel = new BroadcastChannel('socket.runtime.serviceWorker')

if (globalThis.isServiceWorkerScope) {
  globalThis.addEventListener('notificationclose', (event) => {
    for (const notification of observedNotifications) {
      if (notification.id === event.notification.id) {
        notification.dispatchEvent(new Event('close'))
        observedNotifications.delete(notification)
      }
    }
  })
}

export async function showNotification (registration, title, options) {
  if (title && typeof title === 'object') {
    options = title
    title = options.title ?? ''
  }

  const info = registration[Symbol.for('socket.runtime.ServiceWorkerRegistration.info')]

  if (!info) {
    throw new TypeError('Invalid ServiceWorkerRegistration instance given')
  }

  if (!registration.active) {
    throw new TypeError('ServiceWorkerRegistration is not active')
  }

  const query = await permissions.query({ name: 'notifications' })

  if (query.state !== 'granted') {
    throw new NotAllowedError('Operation not permitted')
  }

  // will throw if invalid options are given
  options = new NotificationOptions(options, /* allowServiceWorkerGlobalScope= */ true)
  const nonce = Math.random().toString(16).slice(2)
  const target = globalThis.isServiceWorkerScope ? globalThis : channel
  const message = {
    nonce,
    registration: { id: info.id },
    showNotification: { title, ...options.toJSON() }
  }

  target.postMessage(message)

  await new Promise((resolve, reject) => {
    target.addEventListener('message', function onMessage (event) {
      if (event.data?.nonce === nonce) {
        target.removeEventListener('message', onMessage)
        if (event.data.error) {
          reject(new Error(event.data.error.message))
        } else {
          resolve(event.data.notification)
        }
      }
    })
  })
}

export async function getNotifications (registration, options = null) {
  const info = registration[Symbol.for('socket.runtime.ServiceWorkerRegistration.info')]

  if (!info) {
    throw new TypeError('Invalid ServiceWorkerRegistration instance given')
  }

  if (!registration.active) {
    throw new TypeError('ServiceWorkerRegistration is not active')
  }

  const nonce = Math.random().toString(16).slice(2)
  const target = globalThis.isServiceWorkerScope ? globalThis : channel
  const message = {
    nonce,
    registration: { id: info.id },
    getNotifications: { tag: options?.tag ?? null }
  }

  target.postMessage(message)

  const notifications = await new Promise((resolve, reject) => {
    target.addEventListener('message', function onMessage (event) {
      if (event.data?.nonce === nonce) {
        target.removeEventListener('message', onMessage)
        if (event.data.error) {
          reject(new Error(event.data.message))
        } else {
          resolve(event.data.notifications.map((notification) => new Notification(
            notification.title,
            notification.options,
            notification.data
          )))
        }
      }
    })
  })

  for (const notification of notifications) {
    observedNotifications.add(notification)
  }

  return notifications
}

export default {
  showNotification,
  getNotifications
}
