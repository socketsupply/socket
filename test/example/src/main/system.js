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

const log = s => process.stdout.write(s + '\0')

const write = s => {
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      process.stdout.write(s + '\0', 'utf8', (err) => {
        if (err) return reject(err)
        resolve()
      });
    }, 512) // give the OS time to finalize
  })
}

const api = {}

const exceedsMaxSize = s => {
  if (s.length > 8000) {
    return [
      'Unable to accept payload. Max ipc payload size reached (Exceeds',
      'JSStringGetMaximumUTF8CStringSize), consider writing and reading a file.'
    ].join(' ')
  }
}

console.log = (...args) => log('stdout;' + args.map(v => util.format(v)).join(' '))

api.show = o => {
  const s = new URLSearchParams(o).toString();
  return write(`ipc://show?${s}`)
}

api.navigate = o => {
  const s = new URLSearchParams(o).toString();
  return write(`ipc://navigate?${s}`)
}

api.setTitle = o => {
  const s = new URLSearchParams(o).toString();
  return write(`ipc://title?${s}`)
}

api.setSize = o => {
  const s = new URLSearchParams(o).toString();
  return write(`ipc://size?${s}`)
}

api.setMenu = s => {
  return write(`ipc://setMenu?value=${s.replace(/\n/g ,'%%')}`)
}

api.receive = fn => {
  process.stdin.on('data', async data => {
    const msg = data.split(';')
    if (msg[0] !== 'ipc') return

    let status = msg[1]
    const seq = msg[2]
    const value = msg[3]

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

    if (typeof result === 'object') result = JSON.stringify(result)

    result = encodeURIComponent(result)

    const err = exceedsMaxSize(result)

    if (err) {
      status = 1
      result = err
    }

    const s = new URLSearchParams({
      seq,
      status,
      result
    }).toString();

    log(`ipc://respond?${s}`)
  })
}

api.send = o => {
  let result = ''

  if (typeof o === 'object') result = JSON.stringify(o)

  result = encodeURIComponent(result)

  const err = exceedsMaxSize(result)
  if (err) {
    result = err
  }

  log(result)
}

export default api
