const Uint8ArrayPrototype = Uint8Array.prototype
const TypedArrayPrototype = Object.getPrototypeOf(Uint8ArrayPrototype)
const TypedArray = TypedArrayPrototype.constructor

function isTypedArray (object) {
  return object instanceof TypedArray
}

function isArrayBuffer (object) {
  return object instanceof ArrayBuffer
}

function findMessageTransfers (transfers, object, options = null) {
  if (isTypedArray(object) || ArrayBuffer.isView(object)) {
    add(object.buffer)
  } else if (isArrayBuffer(object)) {
    add(object)
  } else if (object instanceof MessagePort) {
    add(object)
  } else if (Array.isArray(object)) {
    for (const value of object) {
      findMessageTransfers(transfers, value, options)
    }
  } else if (object && typeof object === 'object') {
    for (const key in object) {
      if (
        key.startsWith('__vmScriptReferenceArgs_') &&
        options?.ignoreScriptReferenceArgs === true
      ) {
        continue
      }

      findMessageTransfers(transfers, object[key], options)
    }
  }

  return transfers

  function add (value) {
    if (!transfers.includes(value)) {
      transfers.push(value)
    }
  }
}

class Client extends EventTarget {
  id = null
  port = null

  constructor (id, port) {
    super()
    this.id = id
    this.port = port
    this.onMessage = this.onMessage.bind(this)
    this.port.addEventListener('message', this.onMessage)
  }

  onMessage (event) {
    try {
      this.dispatchEvent(new MessageEvent(event.type, { data: event.data }))
    } catch (err) {
      console.warn(err)
    }
  }

  postMessage (...args) {
    return this.port.postMessage(...args)
  }
}

class Realm {
  /**
   * The `MessagePort` for the VM realm.
   * @type {MessagePort}
   */
  port = null

  /**
   * A reference to the top level worker statae
   * @type {State}
   */
  state = null

  /**
   * Known content worlds that exist in a realm
   * @type {Map<String, World>}
   */
  worlds = new Map()

  constructor (state, port) {
    this.state = state
    this.port = port
  }

  get clients () {
    return this.state.clients
  }

  postMessage (...args) {
    return this.port.postMessage(...args)
  }
}

class State {
  static init () {
    const state = new this()
    state.init()
    return state
  }

  /**
   * All known connected `MessagePort` instances
   * @type {MessagePort[]}
   */
  ports = []

  /**
   * Pending events to be dispatched to realm
   * @type {MessageEvent[]}
   */
  pending = []

  /**
   * The realm for all virtual machines. This is a headless webview
   */
  realm = null

  clients = new Map()

  constructor () {
    this.onConnect = this.onConnect.bind(this)
  }

  init () {
    globalThis.addEventListener('connect', this.onConnect)
  }

  onConnect (event) {
    for (const port of event.ports) {
      this.ports.push(port)
      port.start()
      port.addEventListener('message', this.onPortMessage.bind(this, port))
      port.postMessage('VM_SHARED_WORKER_ACK')
    }
  }

  onPortMessage (port, event) {
    // debug echo
    // port.postMessage(event.data)

    if (event.data?.type === 'terminate-worker') {
      for (const port of this.ports) {
        try {
          port.postMessage({ type: 'terminate-worker' })
        } catch {}
      }

      // timeout so realm can GC
      setTimeout(() => {
        globalThis.close()
      }, 2000)
      return
    }

    if (event.data?.type === 'client') {
      const { id } = event.data
      let client = null

      if (this.clients.has(id)) {
        client = this.clients.get(id)
      } else {
        client = new Client(id, port)
        this.clients.set(id, client)
      }
    }

    if (event.data?.type === 'script' || event.data?.type === 'destroy') {
      const { id } = event.data
      const client = this.clients.get(id)

      if (client) {
        if (!this.realm) {
          this.pending.push(event)
        } else {
          const transfer = []
          findMessageTransfers(transfer, event.data)
          this.realm.postMessage(event.data, { transfer })
        }
      }
    }

    if (event.data?.type === 'result') {
      const { id } = event.data
      const client = this.clients.get(id)

      if (client) {
        const transfer = []
        findMessageTransfers(transfer, event.data)
        client.postMessage(event.data, { transfer })
      }
    }

    if (event.data?.type === 'realm') {
      this.realm = new Realm(this, port)
      if (this.pending.length) {
        for (const pendingEvent of this.pending) {
          const transfer = []
          findMessageTransfers(transfer, pendingEvent.data)
          this.realm.postMessage(pendingEvent.data, { transfer })
        }
        this.pending.splice(0, this.pending.length)
      }
    }
  }
}

if (globalThis.self && !globalThis.window) {
  State.init()
}
