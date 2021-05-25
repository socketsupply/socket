# IPC

The IPC protocol is based on a simple incrementing counter. When the
`send` function is called, a promise is created whos id is the current
counter. When a response is received from the main process, the promse
can be resolved or rejected.

#### What data can be sent and received?

Any stringify-able JSON value can be sent or received.

## Render Process

```js
const result = await window.send('honk')
assert(result === 'goose')
```

## Main process (node.js example)

The spawned process should send and receive json using a simple
incrementing counter. Here is an example [implementation][0].

```js
import { send, receive } from './ipc.js'

receive(async data => {
  if (data === 'honk') send('goose')
})
```

## Implementing the main process IPC in language X

If you want to implement the main process IPC for your language, this provides
defails that you'll want to understand.

### Render Process

When an `main.cc` create an IPC method, a global function is created that when
called, increments a global counter, creates and return a promise. Webkit exposes
`external.invoke` which can send unicode strings. ipc messages should start with
`ipc` and be separated by semi-colons.

```js
const IPC = window._ipc = (window._ipc || { nextSeq: 1 });

window[name] = (value) => {
  const seq = IPC.nextSeq++
  const promise = new Promise((resolve, reject) => {
    IPC[seq] = {
      resolve: resolve,
      reject: reject,
    }
  })

  let encoded

  try {
    encoded = encodeURIComponent(JSON.stringify(value))
  } catch (err) {
    return Promise.reject(err.message)
  }

  window.external.invoke(`ipc;${seq};${name};${encoded}`)
  return promise
}
```

### Main Process

In the main process we want to listen for stdin, here is a minimal
example of an echo server.

```js
process.stdin.resume()
process.stdin.setEncoding('utf8')

process.stdin.on('data', async data => {
  const msg = data.split(';')

  // throw away messages that don't conform to our protocol
  if (msg[0] !== 'ipc') return

  let status = msg[1]
  const seq = msg[2]
  const value = msg[3]

  // don't forget to end the write with a null byte!
  const f = `ipc;${status};${seq};${value}\0`
  process.stdout.write(f)
})
```

[0]:https://github.com/optoolco/opkit/blob/master/test/example/src/main/ipc.js
