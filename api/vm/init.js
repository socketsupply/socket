import application from '../application.js'
import * as vm from '../vm.js'
import hooks from '../hooks.js'

class World extends EventTarget {
  ready = null
  frame = null
  state = null
  id = null

  constructor (state, id) {
    super()
    this.state = state
    this.id = id
    this.frame = createWorld({ id })
    this.ready = new Promise((resolve) => {
      this.frame.contentWindow.addEventListener('load', resolve, { once: true })
    })
  }

  async postMessage (...args) {
    await this.ready
    this.frame.contentWindow.postMessage(...args)
  }

  async destroy () {
    // send destroy signal
    await this.postMessage({ id: this.id, type: 'destroy' })
    // wait for state to emit 'destroyed' on instance, after GC
    await new Promise((resolve) => {
      this.addEventListener('destroyed', resolve, { once: true })
    })
  }
}

class State {
  static init () {
    const state = new State()
    state.init()
    return state
  }

  ports = []
  worker = null

  /**
   * A mapping of client IDs to content worlds.
   * @type {Map<String, World>}
   */
  worlds = new Map()
  clients = new Map()

  constructor () {
    this.onMessage = this.onMessage.bind(this)
    this.onWorkerMessage = this.onWorkerMessage.bind(this)
    this.onWorkerMessageError = this.onWorkerMessageError.bind(this)
  }

  init () {
    globalThis.addEventListener('message', this.onMessage)
    hooks.onReady(async () => {
      this.worker = await vm.getContextWorker()
      this.worker.port.addEventListener('message', this.onWorkerMessage)
      this.worker.port.addEventListener('mesageerror', this.onWorkerMessageError)
      this.worker.port.postMessage({ type: 'realm' })

      const windows = await application.getWindows([], { max: false })
      const currentWindow = await application.getCurrentWindow()

      for (const index in windows) {
        const window = windows[index]
        if (window.index !== currentWindow.index) {
          await currentWindow.send({
            window: window.index,
            event: `vm:${currentWindow.index}:ready`,
            value: {}
          })
        }
      }
    })
  }

  onMessage (event) {
    if (event.data?.type === 'world.result') {
      const transfer = []
      vm.findMessageTransfers(transfer, event.data)
      this.worker.port.postMessage({ ...event.data, type: 'result' }, { transfer })
    }

    if (event.data?.type === 'world.destroy') {
      const { id } = event.data
      const world = this.worlds.get(id)
      if (world) {
        world.dispatchEvent(new Event('destroyed'))
        this.worlds.delete(id)
        if (world.frame?.parentElement) {
          world.frame.parentElement.removeChild(world.frame)
        }
      }
    }
  }

  async onWorkerMessage (event) {
    if (event.data?.type === 'terminate-worker') {
      const pending = []
      for (const world of this.worlds.values()) {
        pending.push(world.destroy())
      }

      await Promise.all(pending)
      const currentWindow = await application.getCurrentWindow()
      await currentWindow.close()
      this.worker = null
    }

    if (event.data?.type === 'script') {
      const { id } = event.data
      const transfer = []
      const world = this.worlds.get(id) ?? new World(this, id)

      if (!this.worlds.has(id)) {
        this.worlds.set(id, world)
      }

      vm.findMessageTransfers(transfer, event.data)

      world.postMessage(event.data, { transfer })
    }

    if (event.data?.type === 'destroy') {
      const { id } = event.data
      const world = this.worlds.get(id)
      if (world) {
        world.destroy()
      }
    }
  }

  onWorkerMessageError (event) {
    globalThis.reportError(
      event.error ??
      new Error('An unknown VM worker error occurred', { cause: event })
    )
  }
}

function createWorld (options) {
  const frame = globalThis.document.createElement('iframe')

  frame.setAttribute('sandbox', 'allow-same-origin allow-scripts')
  frame.src = `${globalThis.origin}/socket/vm/world.html`
  frame.id = options.id

  Object.assign(frame.style, {
    width: 0,
    height: 0,
    display: 'none'
  })

  const target = (
    globalThis.document.head ??
    globalThis.document.body ??
    globalThis.document
  )

  target.appendChild(frame)

  return frame
}

if (globalThis.window === globalThis) {
  State.init()
}
