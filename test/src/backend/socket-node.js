// @ts-check
import { format } from 'node:util'
import { writeSync, fsyncSync } from 'node:fs'
import { EventEmitter } from 'node:events'

const MAX_MESSAGE_KB = 512 * 1024

//
// Exported API
//
class API {
  #buf = ''
  #emitter = new EventEmitter()

  constructor () {
    process.stdin.resume()
    process.stdin.setEncoding('utf8')

    process.stdin.on('data', this.#handleMessage.bind(this))
    process.on('exit', (exitCode) => {
      // TODO: notify the webview that backend is exiting
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
  }

  //
  // Internal API
  //
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
  
    await this.#write(`ipc://send?${s}`)
  }

  async heartbeat () {
    return await this.#write('ipc://heartbeat')
  }

  on (event, cb) {
    this.#emitter.on(event, cb)
  }
}

const api = new API()

export const socket = api
export default api
