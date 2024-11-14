import ipc from './ipc.js'

/**
 * @typedef {{ scheme: string }} GetServiceWorkerOptions

/**
 * @param {GetServiceWorkerOptions} options
 * @return {Promise<ServiceWorker|null>
 */
export async function getServiceWorker (options) {
  const result = await ipc.request('protocol.getServiceWorkerRegistration', {
    scheme: options.scheme
  })

  if (result.err) {
    throw result.err
  }

  if (result.data) {
    const { scope, scriptURL } = result.data
    const registrations = await globalThis.navigator.serviceWorker.getRegistrations()
    for (const registration of registrations) {
      const serviceWorker = (
        registration.active ||
        registration.waiting ||
        registration.installing ||
        null
      )

      if (
        serviceWorker && registration.scope === scope &&
        serviceWorker.scriptURL === scriptURL
      ) {
        return serviceWorker
      }
    }
  }

  return null
}

export default {
  getServiceWorker
}
