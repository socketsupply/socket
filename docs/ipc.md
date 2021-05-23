# IPC

The IPC protocol is based on a simple incrementing counter. When the
`send` function is called, a promise is created whos id is the current
counter. When a response is received from the main process, the promse
can be resolved or rejected.

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

[0]:https://github.com/optoolco/opkit/blob/master/test/example/src/main/ipc.js
