//
// TODO(@heapwolf) publish to npm/github
//
process.stdin.resume()
process.stdin.setEncoding('utf8')

const api = {}

const exceedsMaxSize = s => {
  if (s.length > 8000) {
    return [
      'Unable to accept payload. Max ipc payload size reached (Exceeds',
      'JSStringGetMaximumUTF8CStringSize), consider writing and reading a file.'
    ].join(' ')
  }
}

console.log = (...args) => {
  process.stdout.write('stdout;' + args.join(' '))
}

console.error = (...args) => {
  process.stderr.write(args.join(' '))
}

api.setTitle = s => {
  process.stdout.write(`binding;setTitle;${s}\0`)
}

api.setSize = o => {
  process.stdout.write(`binding;setSize;${o.width}x${o.height}\0`)
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
      console.error('NOPE')
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

    process.stdout.write(`ipc;${status};${seq};${result}\0`)
  })
}

api.send = result => {
  if (typeof result === 'object') result = JSON.stringify(result)

  result = encodeURIComponent(result)

  const err = exceedsMaxSize(result)
  if (err) {
    result = err
  }

  process.stdout.write(Buffer.from(result) + '\0')
}

export default api
