const Uint8ArrayPrototype = Uint8Array.prototype
const TypedArrayPrototype = Object.getPrototypeOf(Uint8ArrayPrototype)
const TypedArray = TypedArrayPrototype.constructor

function isTypedArray (object) {
  return object instanceof TypedArray
}

function isArrayBuffer (object) {
  return object instanceof ArrayBuffer
}

function findMessageTransfers (transfers, object, options = null) {
  if (isTypedArray(object) || ArrayBuffer.isView(object)) {
    add(object.buffer)
  } else if (isArrayBuffer(object)) {
    add(object)
  } else if (object instanceof MessagePort) {
    add(object)
  } else if (Array.isArray(object)) {
    for (const value of object) {
      findMessageTransfers(transfers, value, options)
    }
  } else if (object && typeof object === 'object') {
    for (const key in object) {
      if (
        key.startsWith('__vmScriptReferenceArgs_') &&
        options?.ignoreScriptReferenceArgs === true
      ) {
        continue
      }

      findMessageTransfers(transfers, object[key], options)
    }
  }

  return transfers

  function add (value) {
    if (!transfers.includes(value)) {
      transfers.push(value)
    }
  }
}

const ports = []
globalThis.addEventListener('connect', (event) => {
  for (const port of event.ports) {
    port.start()
    ports.push(port)
    port.addEventListener('message', (event) => {
      for (const p of ports) {
        if (p !== port) {
          const transfer = []
          findMessageTransfers(transfer, event.data)
          p.postMessage(event.data, { transfer })
        }
      }
    })
  }
})
