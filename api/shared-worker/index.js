/* global ErrorEvent */
import application from '../application.js'
import location from '../location.js'
import crypto from '../crypto.js'
import ipc from '../ipc.js'

let contextWindow = null

export const SHARED_WORKER_WINDOW_INDEX = 46
export const SHARED_WORKER_WINDOW_TITLE = 'socket:shared-worker'
export const SHARED_WORKER_WINDOW_PATH = `${location.origin}/socket/shared-worker/index.html`

export const channel = new BroadcastChannel('socket.runtime.sharedWorker')
export const workers = new Map()

channel.addEventListener('message', (event) => {
  if (event.data?.error?.id) {
    const ref = workers.get(event.data.error.id)
    if (ref) {
      const worker = ref.deref()
      if (!worker) {
        workers.delete(event.data.error.id)
      } else {
        worker.dispatchEvent(new ErrorEvent('error', {
          error: new Error(event.data.error.message) || ''
        }))
      }
    }
  }
})

export async function init (sharedWorker, options) {
  await getContextWindow()
  channel.postMessage({
    connect: {
      scriptURL: options.scriptURL,
      name: options.name,
      port: sharedWorker.port,
      id: sharedWorker.id
    }
  })

  workers.set(sharedWorker.id, new WeakRef(sharedWorker))
}

export class SharedWorkerMessagePort extends ipc.IPCMessagePort {
  [Symbol.for('socket.runtime.serialize')] () {
    return {
      ...(super[Symbol.for('socket.runtime.serialize')]()),
      __type__: 'SharedWorkerMessagePort'
    }
  }
}

export class SharedWorker extends EventTarget {
  #id = null
  #port = null
  #ready = null
  #onerror = null

  /**
   * `SharedWorker` class constructor.
   * @param {string} aURL
   * @param {string|object=} [nameOrOptions]
   */
  constructor (aURL, nameOrOptions = null) {
    const url = new URL(aURL, location.origin)
    const id = crypto.murmur3(url.origin + url.pathname)

    super(url.toString(), nameOrOptions)

    this.#id = id
    this.#port = SharedWorkerMessagePort.create({ id })
    this.#ready = init(this, {
      scriptURL: url.toString(),
      name: typeof nameOrOptions === 'string'
        ? nameOrOptions
        : typeof nameOrOptions?.name === 'string'
          ? nameOrOptions.name
          : null
    })
  }

  get onerror () {
    return this.#onerror
  }

  set onerror (onerror) {
    if (typeof this.#onerror === 'function') {
      this.removeEventListener('error', this.#onerror)
    }

    this.#onerror = null

    if (typeof onerror === 'function') {
      this.#onerror = onerror
      this.addEventListener('error', onerror)
    }
  }

  get ready () {
    return this.#ready
  }

  get port () {
    return this.#port
  }

  get id () {
    return this.#id
  }
}

/**
 * Gets the SharedWorker context window.
 * This function will create it if it does not already exist.
 * @return {Promise<import('./window.js').ApplicationWindow}
 */
export async function getContextWindow () {
  if (contextWindow) {
    await contextWindow.ready
    return contextWindow
  }

  const existingContextWindow = await application.getWindow(
    SHARED_WORKER_WINDOW_INDEX,
    { max: false }
  )

  const pendingContextWindow = (
    existingContextWindow ??
    application.createWindow({
      canExit: false,
      //headless: !process.env.SOCKET_RUNTIME_SHARED_WORKER_DEBUG,
      headless: false,
      // @ts-ignore
      //debug: Boolean(process.env.SOCKET_RUNTIME_SHARED_WORKER_DEBUG),
      debug: true,
      index: SHARED_WORKER_WINDOW_INDEX,
      title: SHARED_WORKER_WINDOW_TITLE,
      path: SHARED_WORKER_WINDOW_PATH,
      config: {
        webview_watch_reload: false
      }
    })
  )

  const promises = []
  promises.push(Promise.resolve(pendingContextWindow))

  if (!existingContextWindow) {
    promises.push(new Promise((resolve) => {
      const timeout = setTimeout(resolve, 500)
      channel.addEventListener('message', function onMessage (event) {
        if (event.data?.ready === SHARED_WORKER_WINDOW_INDEX) {
          clearTimeout(timeout)
          resolve(null)
          channel.removeEventListener('message', onMessage)
        }
      })
    }))
  }

  const ready = Promise.all(promises)
  contextWindow = pendingContextWindow
  contextWindow.ready = ready

  await ready
  contextWindow = await pendingContextWindow
  contextWindow.ready = ready

  return contextWindow
}

export default SharedWorker
