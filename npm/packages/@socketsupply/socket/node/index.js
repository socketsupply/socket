// @ts-check
import { format } from 'node:util'
import { writeSync, fsyncSync } from 'node:fs'

const AUTO_CLOSE = process.env.AUTO_CLOSE
const MAX_MESSAGE_KB = 512 * 1024

function isObject (o) {
  return o && typeof o === 'object' && !Array.isArray(o)
}

const write = s => {
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

console.log = (...args) => {
  const s = args.map(v => format(v)).join(' ')
  const enc = encodeURIComponent(s)
  // fs.appendFileSync('tmp.log', s + '\n')
  write(`ipc://stdout?value=${enc}`)
}
console.error = console.log

process.on('exit', (exitCode) => {
  const seq = String(ipc.nextSeq++)

  let value = new URLSearchParams({
    index: '0',
    seq,
    value: String(exitCode)
  }).toString()

  value = value.replace(/\+/g, '%20')

  writeSync(1, `ipc://exit?${value}\n`)
  try {
    fsyncSync(1)
  } catch (_) {
    // fsync(1) can fail in github actions for reasons unclear.
    // maybe the stdout is weird in that environment.
  }
})

//
// Internal IPC API
//
const ipc = { nextSeq: 0 }

ipc.resolve = async (seq, state, value) => {
  const method = !Number(state) ? 'resolve' : 'reject'
  if (!ipc[seq] || !ipc[seq][method]) return

  /**
   * TODO Who handles this error ?
   */
  try {
    await ipc[seq][method](value)
  } finally {
    delete ipc[seq]
  }
}

ipc.request = async (cmd, opts) => {
  const seq = ipc.nextSeq++
  let value = ''

  const promise = new Promise((resolve, reject) => {
    ipc[seq] = {
      resolve: resolve,
      reject: reject
    }
  })

  try {
    if (typeof opts.value === 'object') {
      opts.value = JSON.stringify(opts.value)
    }

    value = new URLSearchParams({
      ...opts,
      index: opts.window || '0',
      seq,
      value: opts.value || '0'
    }).toString()

    value = value.replace(/\+/g, '%20')
  } catch (err) {
    console.error(`Cannot encode request ${err.message} (${value})`)
    return Promise.reject(err)
  }

  await write(`ipc://${cmd}?${value}`)
  return promise
}

ipc.send = async o => {
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
    index: o.window ?? '-1',
    value: o.value
  }).toString()

  s = s.replace(/\+/g, '%20')

  return await write(`ipc://send?${s}`)
}

process.stdin.resume()
process.stdin.setEncoding('utf8')

let buf = ''

async function handleMessage (data) {
  const messages = data.split('\n')

  if (messages.length === 1) {
    buf += data
    return
  }

  const firstMsg = buf + messages[0]
  parse(firstMsg)

  for (let i = 1; i < messages.length - 1; i++) {
    parse(messages[i])
  }

  buf = messages[messages.length - 1]
}

async function receiveOpNode (_command, value) {
  if (value?.method === 'testUncaught') {
    const opts = value.arguments[0]

    console.error('Got an uncaught in test', opts)

    process.nextTick(() => {
      throw new Error('FRONTEND TEST UNCAUGHT: ' + opts.err.message)
    })
  } else if (value?.method === 'testConsole') {
    const opts = value.arguments[0]

    const args = JSON.parse(opts.args)
    const firstArg = args[0]

    console.log(...args)

    if (typeof firstArg !== 'string') {
      // nothing can be done here
      return {}
    }

    let exitCode = -1
    if (firstArg.indexOf('# ok') === 0) {
      exitCode = 0
    } else if (firstArg.indexOf('# fail ') === 0) {
      exitCode = 1
    }

    if (exitCode !== -1 && AUTO_CLOSE !== 'false') {
      // wait some time to let logs flush.
      setTimeout(() => {
        api.exit({ value: exitCode })
      }, 50)
    }

    return {}
  }
}

async function parse (data) {
  let cmd = ''
  let index = '0'
  let seq = '0'
  let state = '0'

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
    cmd = u.host
    seq = o.seq
    index = o.index
    state = o.state || '0'

    if (o.value) {
      value = JSON.parse(o.value)
    }
  } catch (err) {
    const dataStart = data.slice(0, 100)
    const dataEnd = data.slice(data.length - 100)

    console.error(`Unable to parse stdin message ${err.code} ${err.message.slice(0, 100)} (${dataStart}...${dataEnd})`)
    throw new Error(`Unable to parse stdin message ${err.code} ${err.message.slice(0, 20)}`)
  }

  if (cmd === 'resolve') {
    return ipc.resolve(seq, state, value) // being asked to resolve a promise
  }

  let resultObj
  let result = ''

  try {
    if (isObject(value) && Reflect.get(value, 'api') === 'ssc-node') {
      resultObj = await receiveOpNode(cmd, value)
    } else {
      resultObj = await api.receive(cmd, value)
    }
  } catch (err) {
    resultObj = {
      err: { message: err.message }
    }
    state = '1'
  }

  if (resultObj === undefined) {
    resultObj = null
  }

  try {
    result = JSON.stringify(resultObj)
  } catch (err) {
    state = '1'
    result = JSON.stringify({
      err: { message: err.message }
    })
  }

  let s = new URLSearchParams({
    seq,
    state,
    index,
    value: result
  }).toString()

  s = s.replace(/\+/g, '%20')

  await write(`ipc://resolve?${s}`) // asking to resolve a promise
}

process.stdin.on('data', handleMessage)

//
// Exported API
// ---
//
const api = {
  /**
   * @param {{ window: number, event: string, value: any }} o
   */
  send (o) {
    return ipc.send(o)
  },

  heartbeat () {
    return ipc.request('heartbeat', {})
  },

  /**
   * @param {string} command
   * @param {any} value
   */
  receive (command, value) {
    console.error(`Receive Not Implemented.\nCommand: ${command}\nValue: ${value}`)
    return { err: new Error('Not Implemented!') }
  }
}

export const system = api
export default api
