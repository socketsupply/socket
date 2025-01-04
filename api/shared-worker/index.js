/* global XMLHttpRequest, ErrorEvent */
import application from '../application.js'
import serialize from '../internal/serialize.js'
import location from '../location.js'
import process from '../process.js'
import crypto from '../crypto.js'
import client from '../application/client.js'
import ipc from '../ipc.js'

let contextWindow = null

export const SHARED_WORKER_WINDOW_TITLE = 'socket:shared-worker'
export const SHARED_WORKER_WINDOW_PATH = '/socket/shared-worker/index.html'

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
  const currentWindow = await application.getCurrentWindow()
  const window = await getContextWindow()
  await currentWindow.send({
    event: 'connect',
    window: window.index,
    value: {
      connect: {
        scriptURL: options.scriptURL,
        client: client.toJSON(),
        name: options.name,
        port: serialize(ipc.findIPCMessageTransfers(new Set(), sharedWorker.port)),
        id: sharedWorker.id
      }
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
   * @param {string|URL|Blob} aURL
   * @param {string|object=} [nameOrOptions]
   */
  constructor (aURL, nameOrOptions = null) {
    if (typeof aURL === 'string' && !URL.canParse(aURL, location.href)) {
      const blob = new Blob([aURL], { type: 'text/javascript' })
      aURL = URL.createObjectURL(blob).toString()
    } else if (String(aURL).startsWith('blob:')) {
      const request = new XMLHttpRequest()
      request.open('GET', String(aURL), false)
      request.send()

      const blob = new Blob(
        [request.responseText || request.response],
        { type: 'application/javascript' }
      )

      aURL = URL.createObjectURL(blob)
    }

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

  const windows = await application.getWindows([], { max: false })
  let firstInit = true

  for (const window of windows) {
    if (window.title === SHARED_WORKER_WINDOW_TITLE) {
      if (window.location) {
        const url = new URL(window.location.href, globalThis.location.origin)
        if (url.origin === globalThis.location.origin) {
          contextWindow = window
          firstInit = false
        }
      }
    }
  }

  if (!contextWindow) {
    contextWindow = await application.createWindow({
      // @ts-ignore
      canExit: false,
      headless: !process.env.SOCKET_RUNTIME_SHARED_WORKER_DEBUG,
      // @ts-ignore
      debug: Boolean(process.env.SOCKET_RUNTIME_SHARED_WORKER_DEBUG),
      reserved: true,
      index: -1,
      title: SHARED_WORKER_WINDOW_TITLE,
      path: SHARED_WORKER_WINDOW_PATH,
      width: '80%',
      height: '80%',
      config: {
        ...globalThis.__args.config,
        webview_watch_reload: false
      }
    })
  }

  if (!contextWindow.ready) {
    contextWindow.ready = new Promise((resolve) => {
      const timeout = setTimeout(resolve, 500)
      channel.addEventListener('message', function onMessage (event) {
        if (event.data?.ready === contextWindow.index) {
          clearTimeout(timeout)
          resolve(null)
          channel.removeEventListener('message', onMessage)
        }
      })
    })
  }

  if (!process.env.SOCKET_RUNTIME_SHARED_WORKER_DEBUG) {
    await contextWindow.hide()
  }

  await contextWindow.ready
  return contextWindow
}

export default SharedWorker
