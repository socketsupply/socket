//
// TODO(@heapwolf) publish to npm/github as @optoolco/window
//
import fs from 'fs'
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

console.log = (...args) => log('stdout;', args.join(' '))

api.setTitle = s => {
  return write(`binding;setTitle;${s}`)
}

api.setSize = o => {
  return write(`binding;setSize;${o.width};${o.height}`)
}

api.setMenu = s => {
  return write(`binding;setMenu;${s.replace(/\n/g ,'%%')}`)
}

api.receive = fn => {
  process.stdin.on('data', async data => {
    const msg = data.split(';')
    if (msg[0] !== 'ipc') return

    let status = msg[1]
    const seq = msg[2]
    const value = msg[3]

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

    log(`ipc;${status};${seq};${result}`)
  })
}

api.send = result => {
  if (typeof result === 'object') result = JSON.stringify(result)

  result = encodeURIComponent(result)

  const err = exceedsMaxSize(result)
  if (err) {
    result = err
  }

  log(Buffer.from(result))
}

export default api
