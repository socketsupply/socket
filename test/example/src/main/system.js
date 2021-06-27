import util from 'util'
import fs from 'fs'

function installWebView () {
  //
  // this will go away in the near future. WebView2 is a new feature
  // and we want to be sure the user has it, if they don't, download
  // and install it for them.
  //`

  // fetch('https://go.microsoft.com/fwlink/p/?LinkId=2124703')
}

const write = s => {
  process.stdout.write(s + '\0')
  fs.appendFileSync('log.txt', s + '\r\n')
}

console.log = (...args) => {
  const s = args.map(v => util.format(v)).join(' ')
  write(`ipc://stdout?value=${encodeURIComponent(s)}`)
}

//
// Internal IPC API
//
const ipc = { nextSeq: 0 }

ipc.resolve = async (seq, state, value) => {
  const method = !Number(state) ? 'resolve' : 'reject'
  if (!ipc[seq] || !ipc[seq][method]) return

  try {
    await ipc[seq][method](value)
  } catch (err) {
    return Promise.reject(err.message)
  }

  delete ipc[seq];
}

ipc.request = (cmd, opts) => {
  const seq = ipc.nextSeq++
  let value = '0'

  const promise = new Promise((resolve, reject) => {
    ipc[seq] = {
      resolve: resolve,
      reject: reject,
    }
  })

  try {
    if (typeof opts.value === 'object') {
      opts.value = JSON.stringify(opts.value)
    }

    value = new URLSearchParams({
      index: opts.window,
      seq,
      value: opts.value || '0'
    }).toString()
  } catch (err) {
    console.error(`${err.message} (${value})`)
    return Promise.reject(err.message)
  }

  write(`ipc://${cmd}?${value}`)
  return promise
}

ipc.send = o => {
  let value = ''

  try {
    value = JSON.stringify(o.value)
  } catch (err) {
    throw new Error(err.message)
  }

  if (!value || !value.trim()) return

  const s = new URLSearchParams({
    event: o.event,
    index: o.index,
    value
  }).toString()

  const err = exceedsMaxSize(s)

  if (err) {
    result = err
  }

  return write(`ipc://send?${s}`)
}

const exceedsMaxSize = (s = "") => {
  if (s.length > 8000) {
    return [
      'Unable to accept payload. Max ipc payload size reached (Exceeds',
      'JSStringGetMaximumUTF8CStringSize), consider streaming.'
    ].join(' ')
  }
}

process.stdin.resume()
process.stdin.setEncoding('utf8')

process.stdin.on('data', async data => {
  let cmd = ''
  let index = 0
  let seq = 0
  let state = 0
  let value = ''

  try {
    const u = new URL(data)
    const o = Object.fromEntries(u.searchParams)
    cmd = u.host
    seq = o.seq
    index = o.index
    state = o.state || 0

    if (o.value) {
      value = JSON.parse(decodeURIComponent(o.value))
    }
  } catch (err) {
    console.log(`Unable to parse message (${data})`)
    return
  }

  if (cmd === 'resolve') {
    return ipc.resolve(seq, state, value) // being asked to resolve a promise
  }

  let result = ''

  try {
    result = JSON.stringify(await api.receive(cmd, value));
  } catch (err) {
    result = err.message
    state = 1
  }

  const err = exceedsMaxSize(result)
  if (err) {
    state = 1
    result = err
  }

  const s = new URLSearchParams({
    seq,
    state,
    index,
    value: result
  }).toString();

  write(`ipc://resolve?${s}`) // asking to resolve a promise
})

//
// Exported API
// ---
//
const api = {}

api.show = o => ipc.request('show', o)

api.navigate = o => ipc.request('navigate', o)

api.setTitle = o => ipc.request('title', o)

api.setSize = o => ipc.request('sizse', o)

api.setMenu = o => ipc.request('menu', o)

api.send = ipc.send

api.receive = () => "Not Implemented!";

export default api
