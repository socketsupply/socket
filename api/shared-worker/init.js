/* global Worker */
import { channel } from './index.js'
import globals from '../internal/globals.js'
import crypto from '../crypto.js'

export const workers = new Map()
export { channel }

globals.register('SharedWorkerContext.workers', workers)
globals.register('SharedWorkerContext.info', new Map())

channel.addEventListener('message', (event) => {
  if (event.data?.connect) {
    onConnect(event)
  }
})

export class SharedWorkerInstance extends Worker {
  #info = null

  constructor (filename, options) {
    super(filename, {
      name: `SharedWorker (${options?.info?.pathname ?? filename})`,
      ...options,
      [Symbol.for('socket.runtime.internal.worker.type')]: 'sharedWorker'
    })

    this.#info = options?.info ?? null
    this.addEventListener('message', this.onMessage.bind(this))
  }

  get info () {
    return this.#info
  }

  async onMessage (event) {
    if (Array.isArray(event.data.__shared_worker_debug)) {
      const log = document.querySelector('#log')
      if (log) {
        for (const entry of event.data.__shared_worker_debug) {
          const lines = entry.split('\n')
          const span = document.createElement('span')
          let target = span

          for (const line of lines) {
            if (!line) continue
            const item = document.createElement('code')
            item.innerHTML = line
              .replace(/\s/g, '&nbsp;')
              .replace(/\\s/g, ' ')
              .replace(/<anonymous>/g, '&lt;anonymous&gt;')
              .replace(/([a-z|A-Z|_|0-9]+(Error|Exception)):/g, '<span class="red"><b>$1</b>:</span>')

            if (target === span && lines.length > 1) {
              target = document.createElement('details')
              const summary = document.createElement('summary')
              summary.appendChild(item)
              target.appendChild(summary)
              span.appendChild(target)
            } else {
              target.appendChild(item)
            }
          }

          log.appendChild(span)
        }

        log.scrollTop = log.scrollHeight
      }
    }
  }
}

export class SharedWorkerInfo {
  id = null
  port = null
  client = null
  scriptURL = null

  url = null
  hash = null

  constructor (data) {
    for (const key in data) {
      const value = data[key]
      if (key in this) {
        this[key] = value
      }
    }

    const url = new URL(this.scriptURL)
    this.url = url.toString()
    this.hash = crypto.murmur3(url.toString())
  }

  get pathname () {
    return new URL(this.url).pathname
  }
}

export async function onInstall (event) {
  const info = new SharedWorkerInfo(event.data.install)

  if (!info.id || workers.has(info.hash)) {
    return
  }

  const worker = new SharedWorkerInstance('./worker.js', {
    info
  })

  workers.set(info.hash, worker)
  globals.get('SharedWorkerContext.info').set(info.hash, info)
  worker.postMessage({ install: info })
}

export async function onUninstall (event) {
  const info = new SharedWorkerInfo(event.data.uninstall)

  if (!workers.has(info.hash)) {
    return
  }

  const worker = workers.get(info.hash)
  workers.delete(info.hash)

  worker.postMessage({ uninstall: info })
}

export async function onConnect (event) {
  const info = new SharedWorkerInfo(event.data.connect)

  if (!info.id) {
    return
  }

  if (!workers.has(info.hash)) {
    onInstall(new MessageEvent('message', {
      data: {
        install: event.data.connect
      }
    }))

    try {
      await new Promise((resolve, reject) => {
        channel.addEventListener('message', (event) => {
          if (event.data?.error?.id === info.id) {
            reject(new Error(event.data.error.message))
          } else if (event.data?.installed?.id === info.id) {
            resolve()
          }
        })
      })
    } catch (err) {
      console.error(err)
    }
  }

  const worker = workers.get(info.hash)
  worker.postMessage({ connect: info })
}

export default null
