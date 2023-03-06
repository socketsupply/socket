// @ts-check
import { format } from 'node:util'
// import { writeSync, fsyncSync } from 'node:fs'
import { EventEmitter } from 'node:events'

const MAX_MESSAGE_KB = 512 * 1024

//
// Exported API
//
class API {
  static #sscVersionPrefix = '--ssc-version=v'
  static #sscVersionPattern = /v(\d+\.\d+\.\d+)/
  static #minimalMajorVersion = 0
  static #minimalMinorVersion = 1
  static #minimalPatchVersion = 0

  #buf = ''
  #emitter = new EventEmitter()

  constructor () {
    process.stdin.resume()
    process.stdin.setEncoding('utf8')

    process.stdin.on('data', this.#handleMessage.bind(this))
    process.on('exit', (exitCode) => {
      this.#write(`ipc://process.exit?value=${exitCode}`)
    })
    process.on('uncaughtException', (err) => {
      this.#write(`ipc://stderr?value=${err}`)
    })

    // redirect console
    console.log = (...args) => {
      const s = args.map(v => format(v)).join(' ')
      const enc = encodeURIComponent(s)
      // fs.appendFileSync('tmp.log', s + '\n')
      this.#write(`ipc://stdout?value=${enc}`)
    }
    console.error = (...args) => {
      const s = args.map(v => format(v)).join(' ')
      const enc = encodeURIComponent(s)
      // fs.appendFileSync('tmp.log', s + '\n')
      this.#write(`ipc://stderr?value=${enc}`)
    }

    for (const arg of process.argv) {
      if (arg.startsWith(API.#sscVersionPrefix)) {
        const [major, minor, patch] = arg.match(API.#sscVersionPattern)?.[1].split('.').map(Number) ?? [0, 0, 0]
        this.#checkVersion(major, minor, patch)
        break
      }
    }
  }

  //
  // Internal API
  //
  #checkVersion (major, minor, patch) {
    if (major < API.#minimalMajorVersion || minor < API.#minimalMinorVersion || patch < API.#minimalPatchVersion) {
      throw new Error(`socket-node-backend requires at least version 0.1.0, got ${major}.${minor}.${patch}`)
    }
  }

  #parse (data) {
    /** @type {string} */
    let event = ''
    /** @type {string | object} */
    let value = ''

    if (data.length > MAX_MESSAGE_KB) {
      const len = Math.ceil(data.length / 1024)
      process.stderr.write(
        'WARNING: Receiving large message from webview: ' + len + 'kb\n'
      )
      process.stderr.write('RAW MESSAGE: ' + data.slice(0, 512) + '...\n')
    }

    try {
      const u = new URL(data)
      const o = Object.fromEntries(u.searchParams)
      event = o.event || ''

      if (o.value) {
        value = JSON.parse(o.value)
      }
    } catch (err) {
      const dataStart = data.slice(0, 100)
      const dataEnd = data.slice(data.length - 100)
      console.error(`Unable to parse stdin message ${err.code} ${err.message.slice(0, 100)} (${dataStart}...${dataEnd})`)
      throw new Error(`Unable to parse stdin message ${err.code} ${err.message.slice(0, 20)}`)
    }

    this.#emitter.emit(event, value)
  }

  #handleMessage (data) {
    const messages = data.split('\n')

    if (messages.length === 1) {
      this.#buf += data
      return
    }

    const firstMsg = this.#buf + messages[0]
    this.#parse(firstMsg)

    for (let i = 1; i < messages.length - 1; i++) {
      this.#parse(messages[i])
    }

    this.#buf = messages[messages.length - 1]
  }

  #write (s) {
    if (s.includes('\n')) {
      throw new Error('invalid write()')
    }

    if (s.length > MAX_MESSAGE_KB) {
      const len = Math.ceil(s.length / 1024)
      process.stderr.write('WARNING: Sending large message to webview: ' + len + 'kb\n')
      process.stderr.write('RAW MESSAGE: ' + s.slice(0, 512) + '...\n')
    }

    return new Promise(resolve =>
      process.stdout.write(s + '\n', resolve)
    )
  }

  //
  // Public API
  //
  /**
   * @param {{ window: number, event: string, value: any }} o
   */
  async send (o) {
    try {
      // TODO: use structuredClone instead once we are on node 17+
      o = JSON.parse(JSON.stringify(o))
    } catch (err) {
      console.error(`Cannot encode data to send via IPC:\n${err.message}`)
      return Promise.reject(err)
    }

    if (typeof o.value === 'object') {
      o.value = JSON.stringify(o.value)
    }

    let s = new URLSearchParams({
      event: o.event,
      // '-1' means that event will be broadcasted to all windows
      index: o.window.toString() ?? '-1',
      value: o.value
    }).toString()

    s = s.replace(/\+/g, '%20')

    return await this.#write(`ipc://send?${s}`)
  }

  async heartbeat () {
    return await this.#write('ipc://heartbeat')
  }

  // public EventEmitter methods
  addListener (event, cb) {
    this.#emitter.addListener(event, cb)
  }

  on (event, cb) {
    this.#emitter.on(event, cb)
  }

  once (event, cb) {
    this.#emitter.once(event, cb)
  }

  removeListener (event, cb) {
    this.#emitter.removeListener(event, cb)
  }

  removeAllListeners (event) {
    this.#emitter.removeAllListeners(event)
  }

  off (event, cb) {
    this.#emitter.off(event, cb)
  }
}

const api = new API()

export const socket = api
export default api
