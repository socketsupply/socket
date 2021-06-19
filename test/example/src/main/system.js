//
// TODO(@heapwolf) publish to npm/github as @optoolco/window
//
import fs from 'fs'
import util from 'util'

function installWebView () {
  //
  // this will go away in the near future. WebView2 is a new feature
  // and we want to be sure the user has it, if they don't, download
  // and install it for them.
  //`

  // fetch('https://go.microsoft.com/fwlink/p/?LinkId=2124703')
}

process.stdin.resume()
process.stdin.setEncoding('utf8')

const write = s => process.stdout.write(s + '\0')

const api = {}

const exceedsMaxSize = s => {
  if (s.length > 8000) {
    return [
      'Unable to accept payload. Max ipc payload size reached (Exceeds',
      'JSStringGetMaximumUTF8CStringSize), consider streaming.'
    ].join(' ')
  }
}

console.log = (...args) => {
  const s = args.map(v => util.format(v)).join(' ')
  write(`ipc://stdout?value=${encodeURIComponent(s)}`)
}

api.show = o => {
  const s = new URLSearchParams(o)
  return write(`ipc://show?${s}`)
}

api.navigate = o => {
  const s = new URLSearchParams(o)
  return write(`ipc://navigate?${s}`)
}

api.setTitle = o => {
  const s = new URLSearchParams(o)
  return write(`ipc://title?${s}`)
}

api.setSize = o => {
  const s = new URLSearchParams(o)
  return write(`ipc://size?${s}`)
}

api.setMenu = s => {
  s = encodeURIComponent(s)
  return write(`ipc://menu?value=${s}`)
}

api.receive = fn => {
  process.stdin.on('data', async data => {
    let msg

    try {
      msg = Object.fromEntries(new URL(data).searchParams)
    } catch (err) {
      console.log(`Unable to parse message (${data})`)
      return
    }

    let {
      status = 0,
      seq,
      value
    } = msg

    if (process.platform === 'win32') {
      if (value === '0xDEADBEEF') {
        return installWebView()
      }
    }

    let result = ''
    try {
      const json = JSON.parse(decodeURIComponent(value))
      result = await fn(json);
    } catch (err) {
      result = err.message
      status = 1
    }

    value = JSON.stringify(result)

    const err = exceedsMaxSize(value)
    if (err) {
      status = 1
      result = err
    }

    const s = new URLSearchParams({
      seq,
      status,
      value
    }).toString();

    write(`ipc://respond?${s}`)
  })
}

api.send = o => {

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

  write(`ipc://send?${s}`)
}

export default api
